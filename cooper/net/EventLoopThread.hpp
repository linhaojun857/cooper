#ifndef net_EventLoopThread_hpp
#define net_EventLoopThread_hpp

#include <condition_variable>
#include <future>
#include <memory>
#include <mutex>
#include <thread>

#include "cooper/net/EventLoop.hpp"
#include "cooper/util/NonCopyable.hpp"

namespace cooper {
/**
 * @brief This class represents an event loop thread.
 *
 */
class EventLoopThread : NonCopyable {
public:
    explicit EventLoopThread(const std::string& threadName = "EventLoopThread");
    ~EventLoopThread();

    /**
     * @brief Wait for the event loop to exit.
     * @note This method blocks the current thread until the event loop exits.
     */
    void wait();

    /**
     * @brief Get the pointer of the event loop of the thread.
     *
     * @return EventLoop*
     */
    EventLoop* getLoop() const {
        return loop_.get();
    }

    /**
     * @brief Run the event loop of the thread. This method doesn't block the
     * current thread.
     *
     */
    void run();

private:
    // With C++20, use std::atomic<std::shared_ptr<EventLoop>>
    std::shared_ptr<EventLoop> loop_;
    std::mutex loopMutex_;

    std::string loopThreadName_;
    void loopFuncs();
    std::promise<std::shared_ptr<EventLoop>> promiseForLoopPointer_;
    std::promise<int> promiseForRun_;
    std::promise<int> promiseForLoop_;
    std::once_flag once_;
    std::thread thread_;
};

}  // namespace cooper

#endif
