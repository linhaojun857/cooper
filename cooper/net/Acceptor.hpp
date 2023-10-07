#ifndef net_Acceptor_hpp
#define net_Acceptor_hpp

#include <functional>

#include "cooper/net/Channel.hpp"
#include "cooper/net/EventLoop.hpp"
#include "cooper/net/InetAddress.hpp"
#include "cooper/net/Socket.hpp"
#include "cooper/util/NonCopyable.hpp"

namespace cooper {
using NewConnectionCallback = std::function<void(int fd, const InetAddress&)>;
using AcceptorSockOptCallback = std::function<void(int)>;
class Acceptor : NonCopyable {
public:
    Acceptor(EventLoop* loop, const InetAddress& addr, bool reUseAddr = true, bool reUsePort = true);
    ~Acceptor();
    const InetAddress& addr() const {
        return addr_;
    }
    void setNewConnectionCallback(const NewConnectionCallback& cb) {
        newConnectionCallback_ = cb;
    };
    void listen();

    void setBeforeListenSockOptCallback(AcceptorSockOptCallback cb) {
        beforeListenSetSockOptCallback_ = std::move(cb);
    }

    void setAfterAcceptSockOptCallback(AcceptorSockOptCallback cb) {
        afterAcceptSetSockOptCallback_ = std::move(cb);
    }

protected:
    int idleFd_;
    Socket sock_;
    InetAddress addr_;
    EventLoop* loop_;
    NewConnectionCallback newConnectionCallback_;
    Channel acceptChannel_;
    void readCallback();
    AcceptorSockOptCallback beforeListenSetSockOptCallback_;
    AcceptorSockOptCallback afterAcceptSetSockOptCallback_;
};
}  // namespace cooper

#endif
