#include "EventLoopThreadPool.hpp"

using namespace cooper;

EventLoopThreadPool::EventLoopThreadPool(size_t threadNum, const std::string& name) : loopIndex_(0) {
    for (size_t i = 0; i < threadNum; ++i) {
        loopThreadVector_.emplace_back(std::make_shared<EventLoopThread>(name));
    }
}
void EventLoopThreadPool::start() {
    for (unsigned int i = 0; i < loopThreadVector_.size(); ++i) {
        loopThreadVector_[i]->run();
    }
}
// void EventLoopThreadPool::stop(){
//    for(unsigned int i=0;i<loopThreadVector_.size();i++)
//    {
//        loopThreadVector_[i].stop();
//    }
//}
void EventLoopThreadPool::wait() {
    for (unsigned int i = 0; i < loopThreadVector_.size(); ++i) {
        loopThreadVector_[i]->wait();
    }
}
EventLoop* EventLoopThreadPool::getNextLoop() {
    if (loopThreadVector_.size() > 0) {
        size_t index = loopIndex_.fetch_add(1, std::memory_order_relaxed);
        EventLoop* loop = loopThreadVector_[index % loopThreadVector_.size()]->getLoop();
        return loop;
    }
    return nullptr;
}
EventLoop* EventLoopThreadPool::getLoop(size_t id) {
    if (id < loopThreadVector_.size())
        return loopThreadVector_[id]->getLoop();
    return nullptr;
}
std::vector<EventLoop*> EventLoopThreadPool::getLoops() const {
    std::vector<EventLoop*> ret;
    for (auto& loopThread : loopThreadVector_) {
        ret.push_back(loopThread->getLoop());
    }
    return ret;
}
