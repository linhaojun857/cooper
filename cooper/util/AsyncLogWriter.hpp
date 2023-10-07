#ifndef log_LogWriter_hpp
#define log_LogWriter_hpp

#include <thread>

#include "cooper/util/LockFreeQueue.hpp"
#include "cooper/util/Semaphore.hpp"

namespace cooper {

class AsyncLogWriter {
public:
    AsyncLogWriter();
    ~AsyncLogWriter();
    void run();
    void write(const char* msg, int len);
    void flushAll();

private:
    using LogMessage = std::pair<std::string, int>;
    bool exitFlag_{false};
    std::thread thread_;
    Semaphore sem_;
    std::shared_ptr<MpscQueue<LogMessage>> queue_;
};
}  // namespace cooper

#endif
