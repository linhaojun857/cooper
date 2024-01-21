#include "ThreadPool.hpp"

#include <sys/prctl.h>

#include <cassert>

#include "cooper/util/Logger.hpp"

using namespace cooper;

ThreadPool::ThreadPool(size_t threadNum, const std::string& name) : threadNum_(threadNum), name_(name), stop_(false) {
    assert(threadNum > 0);
    for (unsigned int i = 0; i < threadNum_; ++i) {
        threads_.push_back(std::thread(std::bind(&ThreadPool::func, this, i)));
    }
}

void ThreadPool::addTask(const std::function<void()>& task) {
    LOG_TRACE << "copy task into threadPool";
    std::lock_guard<std::mutex> lock(taskMutex_);
    taskQueue_.push(task);
    taskCond_.notify_one();
}

void ThreadPool::addTask(std::function<void()>&& task) {
    LOG_TRACE << "move task into threadPool";
    std::lock_guard<std::mutex> lock(taskMutex_);
    taskQueue_.push(std::move(task));
    taskCond_.notify_one();
}

void ThreadPool::func(int threadNum) {
    char tmpName[32];
    snprintf(tmpName, sizeof(tmpName), "%s%d", name_.c_str(), threadNum);
    ::prctl(PR_SET_NAME, tmpName);
    while (!stop_) {
        std::function<void()> r;
        {
            std::unique_lock<std::mutex> lock(taskMutex_);
            while (!stop_ && taskQueue_.empty()) {
                taskCond_.wait(lock);
            }
            if (!taskQueue_.empty()) {
                LOG_TRACE << "get a new task!";
                r = std::move(taskQueue_.front());
                taskQueue_.pop();
            } else
                continue;
        }
        r();
    }
}

size_t ThreadPool::getTaskCount() {
    std::lock_guard<std::mutex> guard(taskMutex_);
    return taskQueue_.size();
}

void ThreadPool::stop() {
    if (!stop_) {
        stop_ = true;
        taskCond_.notify_all();
        for (auto& t : threads_)
            t.join();
    }
}

ThreadPool::~ThreadPool() {
    stop();
}
