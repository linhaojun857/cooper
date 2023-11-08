#include "HttpServer.hpp"

#include "TcpConnectionImpl.hpp"
#include "cooper/util/Utilities.hpp"

namespace cooper {

HttpServer::HttpServer(uint16_t port) {
    loopThread_.run();
    InetAddress addr(port);
    server_ = std::make_shared<TcpServer>(loopThread_.getLoop(), addr, "HttpServer");
}

void HttpServer::start(int loopNum) {
    server_->setRecvMessageCallback(
        std::bind(&HttpServer::recvMsgCallback, this, std::placeholders::_1, std::placeholders::_2));
    server_->setConnectionCallback([](const TcpConnectionPtr& connPtr) {
        if (connPtr->connected()) {
            LOG_DEBUG << "New connection";
        } else if (connPtr->disconnected()) {
            LOG_DEBUG << "connection disconnected";
        }
    });
    server_->setIoLoopNum(loopNum);
    server_->kickoffIdleConnections(KEEP_ALIVE_TIMEOUT);
    server_->start();
    loopThread_.wait();
}

void HttpServer::addEndpoint(const std::string& method, const std::string& path, const cooper::HttpHandler& handler) {
    if (Http::methods.find(method) == Http::methods.end()) {
        LOG_ERROR << "invalid method: " << method;
        return;
    }
    if (path.empty()) {
        LOG_ERROR << "path is empty";
        return;
    }
    if (getRoutes_.find(path) != getRoutes_.end() || postRoutes_.find(path) != postRoutes_.end()) {
        LOG_ERROR << "path: " << path << " already exists";
        return;
    }
    if (method == "GET") {
        getRoutes_[path] = handler;
    } else if (method == "POST") {
        postRoutes_[path] = handler;
    }
}

bool HttpServer::addMountPoint(const std::string& mountPoint, const std::string& dir, const Headers& headers) {
    if (utils::isDir(dir)) {
        std::string mnt = !mountPoint.empty() ? mountPoint : "/";
        if (!mnt.empty() && mnt[0] == '/') {
            baseDirs_.push_back({mnt, dir, headers});
            return true;
        }
    }
    return false;
}

bool HttpServer::removeMountPoint(const std::string& mountPoint) {
    for (auto it = baseDirs_.begin(); it != baseDirs_.end(); ++it) {
        if (it->mountPoint == mountPoint) {
            baseDirs_.erase(it);
            return true;
        }
    }
    return false;
}

void HttpServer::setFileAuthCallback(const cooper::FileAuthCallback& cb) {
    fileAuthCallback_ = cb;
}

void HttpServer::recvMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer) {
    HttpRequest request;
    HttpResponse response;
    request.conn_ = conn;
    request.buffer_ = buffer;
    ParseContext context;
    context.socketPtr = std::dynamic_pointer_cast<TcpConnectionImpl>(conn)->socketPtr_;
    if (!request.parseRequestStartingLine() || !request.parseHeaders() || !request.parseBody(context)) {
        response.statusCode_ = HttpStatus::CODE_400;
        sendResponse(conn, response);
        conn->forceClose();
        return;
    }
    auto it = keepAliveRequests_.find(conn);
    if (it == keepAliveRequests_.end()) {
        if ((request.version_ == "HTTP/1.0" &&
             request.headers_[HttpHeader::CONNECTION] == HttpHeader::Value::CONNECTION_KEEP_ALIVE) ||
            (request.version_ == "HTTP/1.1" &&
             request.headers_[HttpHeader::CONNECTION] != HttpHeader::Value::CONNECTION_CLOSE)) {
            // open keep-alive
            keepAliveRequests_[conn].first = 0;
            keepAliveRequests_[conn].second = MAX_KEEP_ALIVE_REQUESTS;
        } else {
            // close keep-alive
            keepAliveRequests_[conn].first = 0;
            keepAliveRequests_[conn].second = 0;
        }
    }
    if (!handleFileRequest(request, response)) {
        handleRequest(request, response);
    }
    if (response.statusCode_ != HttpStatus::CODE_200) {
        conn->forceClose();
    }
    keepAliveRequests_[conn].first++;
    if (keepAliveRequests_[conn].first >= keepAliveRequests_[conn].second) {
        conn->forceClose();
        keepAliveRequests_.erase(conn);
    }
}

void HttpServer::handleRequest(const HttpRequest& request, HttpResponse& response) {
    LOG_TRACE << "method: " << request.method_ << ", path: " << request.path_;
    if (request.method_ == "GET") {
        auto it = getRoutes_.find(request.path_);
        if (it == getRoutes_.end()) {
            response.statusCode_ = HttpStatus::CODE_404;
            sendResponse(request.conn_, response);
            return;
        }
        it->second(request, response);
        sendResponse(request.conn_, response);
    } else if (request.method_ == "POST") {
        auto it = postRoutes_.find(request.path_);
        if (it == postRoutes_.end()) {
            response.statusCode_ = HttpStatus::CODE_404;
            sendResponse(request.conn_, response);
            return;
        }
        it->second(request, response);
        sendResponse(request.conn_, response);
    } else {
        response.statusCode_ = HttpStatus::CODE_405;
        sendResponse(request.conn_, response);
    }
}

bool HttpServer::handleFileRequest(const cooper::HttpRequest& request, cooper::HttpResponse& response) {
    if (request.method_ != "GET") {
        return false;
    }
    for (const auto& entry : baseDirs_) {
        if (!request.path_.compare(0, entry.mountPoint.size(), entry.mountPoint)) {
            std::string subPath = "/" + request.path_.substr(entry.mountPoint.size());
            if (utils::isValidPath(subPath)) {
                auto path = entry.baseDir + subPath;
                if (path.back() == '/') {
                    path += "index.html";
                }
                if (utils::isFile(path)) {
                    if (fileAuthCallback_) {
                        if (!fileAuthCallback_(path)) {
                            response.statusCode_ = HttpStatus::CODE_403;
                            sendResponse(request.conn_, response);
                            return true;
                        }
                    }
                    for (const auto& kv : entry.headers) {
                        response.headers_[kv.first] = kv.second;
                    }
                    response.contentWriter_ = std::make_shared<HttpContentWriter>(path, utils::findContentType(path));
                    sendResponse(request.conn_, response);
                    return true;
                }
            }
        }
    }
    return false;
}

bool HttpServer::sendResponse(const cooper::TcpConnectionPtr& conn, cooper::HttpResponse& response) {
    std::string res;
    response.headers_[HttpHeader::SERVER] = HttpHeader::Value::SERVER;
    if (keepAliveRequests_[conn].second == 0) {
        // keep-alive is closed
        std::stringstream ss;
        ss << "timeout=" << KEEP_ALIVE_TIMEOUT << ", max=" << keepAliveRequests_[conn].second;
        response.headers_[HttpHeader::CONNECTION] = ss.str();
    }
    if (!response.body_.empty()) {
        response.headers_[HttpHeader::CONTENT_LENGTH] = std::to_string(response.body_.size());
    }
    if (response.contentWriter_) {
        size_t size = utils::getFileSize(response.contentWriter_->file_);
        response.contentWriter_->size_ = size;
        response.headers_[HttpHeader::CONTENT_TYPE] = response.contentWriter_->contentType_;
        if (size > 0) {
            response.headers_[HttpHeader::CONTENT_LENGTH] = std::to_string(size);
        }
    }
    res += response.version_ + " " + std::to_string(response.statusCode_.code) + " " +
           response.statusCode_.description + "\r\n";
    for (auto& header : response.headers_) {
        res += header.first + ": " + header.second + "\r\n";
    }
    res += "\r\n";
    if (!response.contentWriter_) {
        res += response.body_;
    }
    conn->send(res);
    if (response.contentWriter_) {
        response.contentWriter_->write(conn);
    }
    return true;
}

}  // namespace cooper
