#include "AsyncLogWriter.hpp"

#include <iostream>

namespace cooper {
AsyncLogWriter::AsyncLogWriter() {
    queue_ = std::make_shared<MpscQueue<LogMessage>>();
    thread_ = std::thread(&AsyncLogWriter::run, this);
}

AsyncLogWriter::~AsyncLogWriter() {
    exitFlag_ = true;
    sem_.post();
    thread_.join();
    flushAll();
}

void AsyncLogWriter::run() {
    while (!exitFlag_) {
        sem_.wait();
        flushAll();
    }
}

void AsyncLogWriter::write(const char* msg, int len) {
    queue_->enqueue(LogMessage(msg, len));
    sem_.post();
}

void AsyncLogWriter::flushAll() {
    LogMessage msg;
    while (queue_->dequeue(msg)) {
        fwrite(msg.first.c_str(), 1, msg.second, stdout);
    }
}

}  // namespace cooper
