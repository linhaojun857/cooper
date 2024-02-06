#include "AppTcpServer.hpp"

#include <memory>

#include "cooper/net/TcpConnectionImpl.hpp"
#include "cooper/net/TimerQueue.hpp"

namespace cooper {

AppTcpServer::AppTcpServer(uint16_t port, bool pingPong, size_t pingPongInterval, size_t pingPongTimeout) {
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
    server_->setRecvMessageCallback([this](auto&& PH1, auto&& PH2) {
        recvMsgCallback(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
    server_->setConnectionCallback([this](const TcpConnectionPtr& connPtr) {
        if (connPtr->connected()) {
            LOG_DEBUG << "new connection";
            if (pingPong_) {
                auto pingPongEntry =
                    std::make_shared<PingPongEntry>(pingPongInterval_, pingPongTimeout_, connPtr,
                                                    timingWheelMap_[connPtr->getLoop()], shared_from_this());
                timingWheelMap_[connPtr->getLoop()]->insertEntry(pingPongInterval_, pingPongEntry);
                pingPongEntries_[connPtr] = pingPongEntry;
            }
        } else if (connPtr->disconnected()) {
            LOG_DEBUG << "connection disconnected";
            if (pingPong_) {
                auto it = pingPongEntries_.find(connPtr);
                if (it != pingPongEntries_.end()) {
                    auto pingPongEntry = it->second.lock();
                    if (pingPongEntry) {
                        pingPongEntry->reset();
                    }
                    pingPongEntries_.erase(it);
                }
                auto it2 = kickoffEntries_.find(connPtr);
                if (it2 != kickoffEntries_.end()) {
                    auto kickoffEntry = it2->second.lock();
                    if (kickoffEntry) {
                        kickoffEntry->reset();
                    }
                    kickoffEntries_.erase(it2);
                }
            }
        }
        if (connectionCallback_) {
            connectionCallback_(connPtr);
        }
    });
    server_->setAfterAcceptSockOptCallback(sockOptCallback_);
    server_->setIoLoopNum(loopNum);
    if (pingPong_) {
        auto loops = server_->getIoLoops();
        for (auto loop : loops) {
            auto timingWheel = std::make_shared<TimingWheel>(loop, pingPongInterval_, 1.0F,
                                                             pingPongInterval_ < 500 ? pingPongInterval_ + 1 : 100);
            timingWheelMap_[loop] = timingWheel;
        }
    }
    server_->start();
    loopThread_.wait();
}

void AppTcpServer::stop() {
    for (auto& iter : timingWheelMap_) {
        std::promise<void> pro;
        auto f = pro.get_future();
        iter.second->getLoop()->runInLoop([&iter, &pro]() mutable {
            iter.second.reset();
            pro.set_value();
        });
        f.get();
    }
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

void AppTcpServer::setSockOptCallback(const SockOptCallback& cb) {
    sockOptCallback_ = cb;
}

void AppTcpServer::resetKickoffEntry(const cooper::TcpConnectionPtr& connPtr) {
    auto it = kickoffEntries_.find(connPtr);
    if (it != kickoffEntries_.end()) {
        auto kickoffEntry = it->second.lock();
        if (kickoffEntry) {
            kickoffEntry->reset();
        }
        kickoffEntries_.erase(it);
    }
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
            resetKickoffEntry(conn);
            return;
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
            resetKickoffEntry(conn);
            return;
        }
        auto it = mediaHandlers_.find(type);
        if (it != mediaHandlers_.end()) {
            it->second(conn, str.c_str(), packSize);
        } else {
            LOG_ERROR << "no handler for protocol type:" << type;
        }
    }
}

}  // namespace cooper
