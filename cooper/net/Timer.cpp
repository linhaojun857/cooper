#include "Timer.hpp"

#include "cooper/net/EventLoop.hpp"
#include "cooper/util/Logger.hpp"

namespace cooper {
std::atomic<TimerId> Timer::timersCreated_ = ATOMIC_VAR_INIT(InvalidTimerId);
Timer::Timer(const TimerCallback& cb, const TimePoint& when, const TimeInterval& interval)
    : callback_(cb), when_(when), interval_(interval), repeat_(interval.count() > 0), id_(++timersCreated_) {
}
Timer::Timer(TimerCallback&& cb, const TimePoint& when, const TimeInterval& interval)
    : callback_(std::move(cb)), when_(when), interval_(interval), repeat_(interval.count() > 0), id_(++timersCreated_) {
    // LOG_TRACE<<"Timer move contrustor";
}
void Timer::run() const {
    callback_();
}
void Timer::restart(const TimePoint& now) {
    if (repeat_) {
        when_ = now + interval_;
    } else
        when_ = std::chrono::steady_clock::now();
}
bool Timer::operator<(const Timer& t) const {
    return when_ < t.when_;
}
bool Timer::operator>(const Timer& t) const {
    return when_ > t.when_;
}
}  // namespace cooper
