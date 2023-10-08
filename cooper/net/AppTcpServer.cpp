#include "AppTcpServer.hpp"

#include <memory>

#include "cooper/net/TcpConnectionImpl.hpp"
#include "cooper/net/TimerQueue.hpp"

namespace cooper {

AppTcpServer::AppTcpServer(uint16_t port, bool pingPong, double pingPongInterval, size_t pingPongTimeout) {
    loopThread_.run();
    InetAddress addr(port);
    pingPong_ = pingPong;
    pingPongInterval_ = pingPongInterval;
    pingPongTimeout_ = pingPongTimeout;
    server_ = std::make_shared<TcpServer>(loopThread_.getLoop(), addr, "AppTcpServer");
}

AppTcpServer::~AppTcpServer() {
    LOG_TRACE << "AppTcpServer::~AppTcpServer...";
}

void AppTcpServer::start(int loopNum) {
    server_->setRecvMessageCallback(
        std::bind(&AppTcpServer::recvMsgCallback, this, std::placeholders::_1, std::placeholders::_2));
    server_->setConnectionCallback([this](const TcpConnectionPtr& connPtr) {
        if (connPtr->connected()) {
            LOG_DEBUG << "New connection";
            if (pingPong_) {
                auto loop = connPtr->getLoop();
                auto timingWheel = server_->timingWheelMap_[connPtr->getLoop()];
                auto connImpPtr = std::dynamic_pointer_cast<TcpConnectionImpl>(connPtr);
                connImpPtr->timingWheelWeakPtr_ = timingWheel;
                // make sure Socket can be destructed successfully
                std::weak_ptr<TcpConnection> connWeakPtr = connPtr;
                auto timerId = loop->runEvery(pingPongInterval_, [this, timingWheel, connWeakPtr]() {
                    auto connPtr = connWeakPtr.lock();
                    json j;
                    j["type"] = PING_TYPE;
                    connPtr->sendJson(j);
                    auto entry = std::make_shared<TcpConnectionImpl::KickoffEntry>(connPtr);
                    std::dynamic_pointer_cast<TcpConnectionImpl>(connPtr)->kickoffEntry_ = entry;
                    timingWheel->insertEntry(pingPongTimeout_, entry);
                });
                timerIds_[connPtr] = timerId;
            }
        } else if (connPtr->disconnected()) {
            LOG_DEBUG << "connection disconnected";
            if (pingPong_) {
                auto loop = connPtr->getLoop();
                loop->invalidateTimer(timerIds_[connPtr]);
                timerIds_.erase(connPtr);
            }
        }
    });
    server_->setIoLoopNum(loopNum);
    if (pingPong_) {
        for (EventLoop* loop : server_->ioLoops_) {
            server_->timingWheelMap_[loop] = std::make_shared<TimingWheel>(
                loop, pingPongTimeout_, 1.0F, pingPongTimeout_ < 500 ? pingPongTimeout_ + 1 : 100);
        }
    }
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

void AppTcpServer::resetPingPongEntry(const cooper::TcpConnectionPtr& connPtr) {
    auto connImpPtr = std::dynamic_pointer_cast<TcpConnectionImpl>(connPtr);
    auto entry = connImpPtr->kickoffEntry_.lock();
    entry->reset();
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
        if (type == PONG_TYPE && pingPong_) {
            resetPingPongEntry(conn);
        }
        auto it = handlers_.find(type);
        if (it != handlers_.end()) {
            it->second(conn, j);
        } else {
            LOG_ERROR << "no handler for protocol type:" << type;
        }
    }
}

}  // namespace cooper
