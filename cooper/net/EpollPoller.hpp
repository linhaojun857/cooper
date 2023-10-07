#ifndef net_EpollPoller_hpp
#define net_EpollPoller_hpp

#include <map>
#include <memory>

#include "cooper/net/EventLoop.hpp"
#include "cooper/net/Poller.hpp"
#include "cooper/util/NonCopyable.hpp"
using EventList = std::vector<struct epoll_event>;
namespace cooper {
class Channel;

class EpollPoller : public Poller {
public:
    explicit EpollPoller(EventLoop* loop);
    virtual ~EpollPoller();
    virtual void poll(int timeoutMs, ChannelList* activeChannels) override;
    virtual void updateChannel(Channel* channel) override;
    virtual void removeChannel(Channel* channel) override;

private:
    static const int kInitEventListSize = 16;

    int epollfd_;
    EventList events_;
    void update(int operation, Channel* channel);
#ifndef NDEBUG
    using ChannelMap = std::map<int, Channel*>;
    ChannelMap channels_;
#endif
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
};
}  // namespace cooper

#endif
