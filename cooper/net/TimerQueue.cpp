#include "TimerQueue.hpp"

#include <sys/timerfd.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

#include "cooper/net/Channel.hpp"
#include "cooper/net/EventLoop.hpp"

using namespace cooper;
static int createTimerfd() {
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timerfd < 0) {
        std::cerr << "create timerfd failed!" << std::endl;
    }
    return timerfd;
}

static struct timespec howMuchTimeFromNow(const TimePoint& when) {
    auto microSeconds =
        std::chrono::duration_cast<std::chrono::microseconds>(when - std::chrono::steady_clock::now()).count();
    if (microSeconds < 100) {
        microSeconds = 100;
    }
    struct timespec ts;
    ts.tv_sec = static_cast<time_t>(microSeconds / 1000000);
    ts.tv_nsec = static_cast<long>((microSeconds % 1000000) * 1000);
    return ts;
}
static void resetTimerfd(int timerfd, const TimePoint& expiration) {
    // wake up loop by timerfd_settime()
    struct itimerspec newValue;
    struct itimerspec oldValue;
    memset(&newValue, 0, sizeof(newValue));
    memset(&oldValue, 0, sizeof(oldValue));
    newValue.it_value = howMuchTimeFromNow(expiration);
    int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
    if (ret) {
        // LOG_SYSERR << "timerfd_settime()";
    }
}
static void readTimerfd(int timerfd, const TimePoint&) {
    uint64_t howmany;
    ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
    if (n != sizeof howmany) {
        LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
    }
}

void TimerQueue::handleRead() {
    loop_->assertInLoopThread();
    const auto now = std::chrono::steady_clock::now();
    readTimerfd(timerfd_, now);

    std::vector<TimerPtr> expired = getExpired(now);

    callingExpiredTimers_ = true;
    // cancelingTimers_.clear();
    // safe to callback outside critical section
    for (auto const& timerPtr : expired) {
        if (timerIdSet_.find(timerPtr->id()) != timerIdSet_.end()) {
            timerPtr->run();
        }
    }
    callingExpiredTimers_ = false;

    reset(expired, now);
}

///////////////////////////////////////
TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(createTimerfd()),
      timerfdChannelPtr_(new Channel(loop, timerfd_)),
      timers_(),
      callingExpiredTimers_(false) {
    timerfdChannelPtr_->setReadCallback(std::bind(&TimerQueue::handleRead, this));
    // we are always reading the timerfd, we disarm it with timerfd_settime.
    timerfdChannelPtr_->enableReading();
}
void TimerQueue::reset() {
    loop_->runInLoop([this]() {
        timerfdChannelPtr_->disableAll();
        timerfdChannelPtr_->remove();
        close(timerfd_);
        timerfd_ = createTimerfd();
        timerfdChannelPtr_ = std::make_shared<Channel>(loop_, timerfd_);
        timerfdChannelPtr_->setReadCallback(std::bind(&TimerQueue::handleRead, this));
        // we are always reading the timerfd, we disarm it with timerfd_settime.
        timerfdChannelPtr_->enableReading();
        if (!timers_.empty()) {
            const auto nextExpire = timers_.top()->when();
            resetTimerfd(timerfd_, nextExpire);
        }
    });
}
TimerQueue::~TimerQueue() {
    auto chlPtr = timerfdChannelPtr_;
    auto fd = timerfd_;
    loop_->runInLoop([chlPtr, fd]() {
        chlPtr->disableAll();
        chlPtr->remove();
        ::close(fd);
    });
}

TimerId TimerQueue::addTimer(const TimerCallback& cb, const TimePoint& when, const TimeInterval& interval) {
    std::shared_ptr<Timer> timerPtr = std::make_shared<Timer>(cb, when, interval);

    loop_->runInLoop([this, timerPtr]() {
        addTimerInLoop(timerPtr);
    });
    return timerPtr->id();
}
TimerId TimerQueue::addTimer(TimerCallback&& cb, const TimePoint& when, const TimeInterval& interval) {
    std::shared_ptr<Timer> timerPtr = std::make_shared<Timer>(std::move(cb), when, interval);

    loop_->runInLoop([this, timerPtr]() {
        addTimerInLoop(timerPtr);
    });
    return timerPtr->id();
}
void TimerQueue::addTimerInLoop(const TimerPtr& timer) {
    loop_->assertInLoopThread();
    timerIdSet_.insert(timer->id());
    if (insert(timer)) {
        // the earliest timer changed
        resetTimerfd(timerfd_, timer->when());
    }
}

void TimerQueue::invalidateTimer(TimerId id) {
    loop_->runInLoop([this, id]() {
        timerIdSet_.erase(id);
    });
}

bool TimerQueue::insert(const TimerPtr& timerPtr) {
    loop_->assertInLoopThread();
    bool earliestChanged = false;
    if (timers_.size() == 0 || *timerPtr < *timers_.top()) {
        earliestChanged = true;
    }
    timers_.push(timerPtr);
    // std::cout<<"after push new
    // timer:"<<timerPtr->when().microSecondsSinceEpoch()/1000000<<std::endl;
    return earliestChanged;
}

std::vector<TimerPtr> TimerQueue::getExpired(const TimePoint& now) {
    std::vector<TimerPtr> expired;
    while (!timers_.empty()) {
        if (timers_.top()->when() < now) {
            expired.push_back(timers_.top());
            timers_.pop();
        } else
            break;
    }
    return expired;
}
void TimerQueue::reset(const std::vector<TimerPtr>& expired, const TimePoint& now) {
    loop_->assertInLoopThread();
    for (auto const& timerPtr : expired) {
        auto iter = timerIdSet_.find(timerPtr->id());
        if (iter != timerIdSet_.end()) {
            if (timerPtr->isRepeat()) {
                timerPtr->restart(now);
                insert(timerPtr);
            } else {
                timerIdSet_.erase(iter);
            }
        }
    }
    if (!timers_.empty()) {
        const auto nextExpire = timers_.top()->when();
        resetTimerfd(timerfd_, nextExpire);
    }
}
