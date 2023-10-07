#ifndef net_Timer_hpp
#define net_Timer_hpp

#include <atomic>
#include <chrono>
#include <functional>
#include <iostream>

#include "cooper/net/CallBacks.hpp"
#include "cooper/util/NonCopyable.hpp"

namespace cooper {
using TimerId = uint64_t;
using TimePoint = std::chrono::steady_clock::time_point;
using TimeInterval = std::chrono::microseconds;
class Timer : public NonCopyable {
public:
    Timer(const TimerCallback& cb, const TimePoint& when, const TimeInterval& interval);
    Timer(TimerCallback&& cb, const TimePoint& when, const TimeInterval& interval);
    ~Timer() {
        //   std::cout<<"Timer unconstract!"<<std::endl;
    }
    void run() const;
    void restart(const TimePoint& now);
    bool operator<(const Timer& t) const;
    bool operator>(const Timer& t) const;
    const TimePoint& when() const {
        return when_;
    }
    bool isRepeat() {
        return repeat_;
    }
    TimerId id() {
        return id_;
    }

private:
    TimerCallback callback_;
    TimePoint when_;
    const TimeInterval interval_;
    const bool repeat_;
    const TimerId id_;
    static std::atomic<TimerId> timersCreated_;
};

}  // namespace cooper

#endif
