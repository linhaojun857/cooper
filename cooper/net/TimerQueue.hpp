#ifndef net_TimerQueue_hpp
#define net_TimerQueue_hpp

#include <atomic>
#include <memory>
#include <queue>
#include <unordered_set>

#include "cooper/net/CallBacks.hpp"
#include "cooper/net/Timer.hpp"
#include "cooper/util/NonCopyable.hpp"
namespace cooper {
// class Timer;
class EventLoop;
class Channel;
using TimerPtr = std::shared_ptr<Timer>;
struct TimerPtrComparer {
    bool operator()(const TimerPtr& x, const TimerPtr& y) const {
        return *x > *y;
    }
};

class TimerQueue : NonCopyable {
public:
    explicit TimerQueue(EventLoop* loop);
    ~TimerQueue();
    TimerId addTimer(const TimerCallback& cb, const TimePoint& when, const TimeInterval& interval);
    TimerId addTimer(TimerCallback&& cb, const TimePoint& when, const TimeInterval& interval);
    void addTimerInLoop(const TimerPtr& timer);
    void invalidateTimer(TimerId id);
    void reset();

protected:
    EventLoop* loop_;
    int timerfd_;
    std::shared_ptr<Channel> timerfdChannelPtr_;
    void handleRead();
    std::priority_queue<TimerPtr, std::vector<TimerPtr>, TimerPtrComparer> timers_;

    bool callingExpiredTimers_;
    bool insert(const TimerPtr& timePtr);
    std::vector<TimerPtr> getExpired();
    void reset(const std::vector<TimerPtr>& expired, const TimePoint& now);
    std::vector<TimerPtr> getExpired(const TimePoint& now);

private:
    std::unordered_set<uint64_t> timerIdSet_;
};
}  // namespace cooper

#endif
