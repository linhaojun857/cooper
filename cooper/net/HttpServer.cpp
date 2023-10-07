#include "HttpServer.hpp"

#include "cooper/util/Utilities.hpp"

namespace cooper {

HttpServer::HttpServer(uint16_t port) {
    loopThread_.run();
    InetAddress addr(port);
    server_ = std::make_shared<TcpServer>(loopThread_.getLoop(), addr, "HttpServer");
    server_->kickoffIdleConnections(KEEP_ALIVE_TIMEOUT);
}
void HttpServer::start(int loopNum) {
    server_->setRecvMessageCallback(
        std::bind(&HttpServer::recvMsgCallback, this, std::placeholders::_1, std::placeholders::_2));
    server_->setConnectionCallback([this](const TcpConnectionPtr& connPtr) {
        if (connPtr->connected()) {
            keepAliveRequests_[connPtr] = 0;
            LOG_DEBUG << "New connection";
        } else if (connPtr->disconnected()) {
            keepAliveRequests_.erase(connPtr);
            LOG_DEBUG << "connection disconnected";
        }
    });
    server_->setIoLoopNum(loopNum);
    server_->start();
    loopThread_.wait();
}

void HttpServer::stop() {
    server_->stop();
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

void HttpServer::recvMsgCallback(const TcpConnectionPtr& conn, MsgBuffer* buffer) {
    HttpRequest request;
    HttpResponse response;
    request.conn_ = conn;
    request.buffer_ = buffer;
    LOG_INFO << "recv msg: \n" << std::string(buffer->peek(), buffer->readableBytes());
    if (!request.parseRequestStartingLine() || !request.parseHeaders() || !request.parseBody()) {
        response.statusCode_ = HttpStatus::CODE_400;
        sendResponse(conn, response);
        conn->forceClose();
        return;
    }
    if (!handleFileRequest(request, response)) {
        handleRequest(request, response);
    }
    if (response.statusCode_ != HttpStatus::CODE_200) {
        conn->forceClose();
    }
    keepAliveRequests_[conn]++;
    if (keepAliveRequests_[conn] >= MAX_KEEP_ALIVE_REQUESTS) {
        conn->forceClose();
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

}  // namespace cooper
