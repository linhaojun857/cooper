#ifndef util_ThreadPool_hpp
#define util_ThreadPool_hpp

#include <atomic>
#include <condition_variable>
#include <functional>
#include <queue>
#include <string>
#include <thread>

namespace cooper {

class ThreadPool {
public:
    /**
     * @brief Construct a new ThreadPool instance.
     *
     * @param threadNum The number of threads in the threadPool.
     * @param name The name of the pool.
     */
    ThreadPool(size_t threadNum, const std::string& name);

    /**
     * @brief Add a task to the threadPool.
     *
     * @param task
     */
    virtual void addTask(const std::function<void()>& task);
    virtual void addTask(std::function<void()>&& task);

    /**
     * @brief Get the name of the threadPool.
     *
     * @return std::string
     */
    virtual std::string getName() const {
        return name_;
    };

    /**
     * @brief Get the number of tasks to be executed in the threadPool.
     *
     * @return size_t
     */
    size_t getTaskCount();

    /**
     * @brief Stop all threads in the threadPool.
     *
     */
    void stop();

    ~ThreadPool();

private:
    void func(int threadNum);

private:
    size_t threadNum_;
    std::string name_;

    std::queue<std::function<void()>> taskQueue_;
    std::vector<std::thread> threads_;

    std::mutex taskMutex_;
    std::condition_variable taskCond_;
    std::atomic_bool stop_;
};

}  // namespace cooper
#endif
