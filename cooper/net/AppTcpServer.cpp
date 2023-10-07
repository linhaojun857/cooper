#include "AppTcpServer.hpp"

namespace cooper {

AppTcpServer::AppTcpServer(uint16_t port) {
    loopThread_.run();
    InetAddress addr(port);
    server_ = std::make_shared<TcpServer>(loopThread_.getLoop(), addr, "AppTcpServer");
}

AppTcpServer::~AppTcpServer() {
    LOG_TRACE << "AppTcpServer::~AppTcpServer...";
}

void AppTcpServer::start(int loopNum) {
    server_->setRecvMessageCallback(
        std::bind(&AppTcpServer::recvMsgCallback, this, std::placeholders::_1, std::placeholders::_2));
    server_->setConnectionCallback([](const TcpConnectionPtr& connPtr) {
        if (connPtr->connected()) {
            LOG_DEBUG << "New connection";
        } else if (connPtr->disconnected()) {
            LOG_DEBUG << "connection disconnected";
        }
    });
    server_->setIoLoopNum(loopNum);
    server_->start();
    loopThread_.wait();
}

void AppTcpServer::stop() {
    server_->stop();
}

void AppTcpServer::registerProtocolHandler(cooper::AppTcpServer::protocolType header,
                                           const cooper::AppTcpServer::Handler& handler) {
    handlers_[header] = handler;
}

void AppTcpServer::recvMsgCallback(const cooper::TcpConnectionPtr& conn, cooper::MsgBuffer* buffer) {
    LOG_TRACE << "recvMsgCallback";
    auto packSize = *(static_cast<const uint32_t*>((void*)buffer->peek()));
    if (buffer->readableBytes() < packSize) {
        return;
    } else {
        buffer->retrieve(sizeof(packSize));
        auto str = buffer->read(packSize);
        auto j = json::parse(str);
        auto type = j["type"].get<protocolType>();
        auto it = handlers_.find(type);
        if (it != handlers_.end()) {
            it->second(conn, j);
        } else {
            LOG_ERROR << "no handler for protocol type:" << type;
        }
    }
}

}  // namespace cooper
