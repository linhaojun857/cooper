#include "Poller.hpp"

#include "cooper/net/EpollPoller.hpp"

using namespace cooper;
Poller* Poller::newPoller(EventLoop* loop) {
    return new EpollPoller(loop);
}
