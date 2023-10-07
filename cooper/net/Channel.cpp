#include "Channel.hpp"

#include <poll.h>

#include <iostream>

#include "cooper/net/EventLoop.hpp"
namespace cooper {
const int Channel::kNoneEvent = 0;

const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false) {
}

void Channel::remove() {
    assert(events_ == kNoneEvent);
    addedToLoop_ = false;
    loop_->removeChannel(this);
}

void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::handleEvent() {
    // LOG_TRACE<<"revents_="<<revents_;
    if (events_ == kNoneEvent)
        return;
    if (tied_) {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard) {
            handleEventSafely();
        }
    } else {
        handleEventSafely();
    }
}
void Channel::handleEventSafely() {
    if (eventCallback_) {
        eventCallback_();
        return;
    }
    if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
        // LOG_TRACE<<"handle close";
        if (closeCallback_)
            closeCallback_();
    }
    if (revents_ & (POLLNVAL | POLLERR)) {
        // LOG_TRACE<<"handle error";
        if (errorCallback_)
            errorCallback_();
    }
    if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
        // LOG_TRACE<<"handle read";
        if (readCallback_)
            readCallback_();
    }

    if (revents_ & POLLOUT) {
        // LOG_TRACE<<"handle write";
        if (writeCallback_)
            writeCallback_();
    }
}

}  // namespace cooper
