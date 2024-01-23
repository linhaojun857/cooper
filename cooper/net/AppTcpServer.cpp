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

void AppTcpServer::setMode(ModeType mode) {
    assert(mode == BUSINESS_MODE || mode == MEDIA_MODE);
    mode_ = mode;
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
        if (connectionCallback_) {
            connectionCallback_(connPtr);
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

void AppTcpServer::registerBusinessHandler(ProtocolType type, const BusinessHandler& handler) {
    assert(mode_ == BUSINESS_MODE);
    businessHandlers_[type] = handler;
}

void AppTcpServer::registerMediaHandler(ProtocolType type, const MediaHandler& handler) {
    assert(mode_ == MEDIA_MODE);
    mediaHandlers_[type] = handler;
}

void AppTcpServer::setConnectionCallback(const ConnectionCallback& cb) {
    connectionCallback_ = cb;
}

void AppTcpServer::resetPingPongEntry(const cooper::TcpConnectionPtr& connPtr) {
    auto connImpPtr = std::dynamic_pointer_cast<TcpConnectionImpl>(connPtr);
    auto entry = connImpPtr->kickoffEntry_.lock();
    entry->reset();
}

void AppTcpServer::recvMsgCallback(const cooper::TcpConnectionPtr& conn, cooper::MsgBuffer* buffer) {
    if (mode_ == BUSINESS_MODE) {
        recvBusinessMsgCallback(conn, buffer);
    } else if (mode_ == MEDIA_MODE) {
        recvMediaMsgCallback(conn, buffer);
    }
}

void AppTcpServer::recvBusinessMsgCallback(const cooper::TcpConnectionPtr& conn, cooper::MsgBuffer* buffer) {
    auto packSize = *(static_cast<const uint32_t*>((void*)buffer->peek()));
    if (buffer->readableBytes() < packSize) {
        return;
    } else {
        buffer->retrieve(sizeof(packSize));
        auto str = buffer->read(packSize);
        auto j = json::parse(str);
        auto type = j["type"].get<ProtocolType>();
        if (type == PONG_TYPE && pingPong_) {
            resetPingPongEntry(conn);
        }
        auto it = businessHandlers_.find(type);
        if (it != businessHandlers_.end()) {
            it->second(conn, j);
        } else {
            LOG_ERROR << "no handler for protocol type:" << type;
        }
    }
}

void AppTcpServer::recvMediaMsgCallback(const cooper::TcpConnectionPtr& conn, cooper::MsgBuffer* buffer) {
    auto packSize = *(static_cast<const uint32_t*>((void*)buffer->peek()));
    if (buffer->readableBytes() < packSize) {
        return;
    } else {
        buffer->retrieve(sizeof(packSize));
        auto str = buffer->read(packSize);
        auto type = *(static_cast<const ProtocolType*>((void*)str.c_str()));
        if (type == PONG_TYPE && pingPong_) {
            resetPingPongEntry(conn);
        }
        auto it = mediaHandlers_.find(type);
        if (it != mediaHandlers_.end()) {
            it->second(conn, str.c_str() + sizeof(type), packSize - sizeof(type));
        } else {
            LOG_ERROR << "no handler for protocol type:" << type;
        }
    }
}

}  // namespace cooper
