#ifndef net_Poller_hpp
#define net_Poller_hpp

#include <map>
#include <memory>

#include "cooper/net/EventLoop.hpp"
#include "cooper/util/NonCopyable.hpp"

namespace cooper {
class Channel;
class Poller : NonCopyable {
public:
    explicit Poller(EventLoop* loop) : ownerLoop_(loop){};
    virtual ~Poller() {
    }
    void assertInLoopThread() {
        ownerLoop_->assertInLoopThread();
    }
    virtual void poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
    virtual void resetAfterFork() {
    }
    static Poller* newPoller(EventLoop* loop);

private:
    EventLoop* ownerLoop_;
};
}  // namespace cooper

#endif
