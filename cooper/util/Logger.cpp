#include "Logger.hpp"

#include <sys/syscall.h>
#include <unistd.h>

#include <cstdio>
#include <sstream>
#include <thread>

namespace cooper {
// helper class for known string length at compile time
class T {
public:
    T(const char* str, unsigned len) : str_(str), len_(len) {
        assert(strlen(str) == len_);
    }

    const char* str_;
    const unsigned len_;
};

const char* strerror_tl(int savedErrno) {
    return strerror(savedErrno);
}

inline LogStream& operator<<(LogStream& s, T v) {
    s.append(v.str_, v.len_);
    return s;
}

inline LogStream& operator<<(LogStream& s, const Logger::SourceFile& v) {
    s.append(v.data_, v.size_);
    return s;
}

}  // namespace cooper
using namespace cooper;

static thread_local uint64_t lastSecond_{0};
static thread_local char lastTimeString_[32] = {0};
static thread_local pid_t threadId_{0};

//   static thread_local LogStream logStream_;

void Logger::formatTime() {
    uint64_t now = static_cast<uint64_t>(date_.secondsSinceEpoch());
    uint64_t microSec =
        static_cast<uint64_t>(date_.microSecondsSinceEpoch() - date_.roundSecond().microSecondsSinceEpoch());
    if (now != lastSecond_) {
        lastSecond_ = now;
        if (displayLocalTime_()) {
            strncpy(lastTimeString_, date_.toFormattedStringLocal(false).c_str(), sizeof(lastTimeString_) - 1);
        } else {
            strncpy(lastTimeString_, date_.toFormattedString(false).c_str(), sizeof(lastTimeString_) - 1);
        }
    }
    logStream_ << T(lastTimeString_, 17);
    char tmp[32];
    if (displayLocalTime_()) {
        snprintf(tmp, sizeof(tmp), ".%06llu ", static_cast<long long unsigned int>(microSec));
        logStream_ << T(tmp, 8);
    } else {
        snprintf(tmp, sizeof(tmp), ".%06llu UTC ", static_cast<long long unsigned int>(microSec));
        logStream_ << T(tmp, 12);
    }
    if (threadId_ == 0)
        threadId_ = static_cast<pid_t>(::syscall(SYS_gettid));
    logStream_ << threadId_;
}
static const char* logLevelStr[Logger::LogLevel::kNumberOfLogLevels] = {
    " TRACE ", " DEBUG ", " INFO  ", " WARN  ", " ERROR ", " FATAL ",
};

Logger::Logger(SourceFile file, int line) : sourceFile_(file), fileLine_(line), level_(kInfo) {
    formatTime();
    logStream_ << T(logLevelStr[level_], 7);
}
Logger::Logger(SourceFile file, int line, LogLevel level) : sourceFile_(file), fileLine_(line), level_(level) {
    formatTime();
    logStream_ << T(logLevelStr[level_], 7);
}
Logger::Logger(SourceFile file, int line, LogLevel level, const char* func)
    : sourceFile_(file), fileLine_(line), level_(level) {
    formatTime();
    logStream_ << T(logLevelStr[level_], 7) << "[" << func << "] ";
}
Logger::Logger(SourceFile file, int line, bool) : sourceFile_(file), fileLine_(line), level_(kFatal) {
    formatTime();
    logStream_ << T(logLevelStr[level_], 7);
    if (errno != 0) {
        logStream_ << strerror_tl(errno) << " (errno=" << errno << ") ";
    }
}

// LOG_COMPACT
Logger::Logger() : level_(kInfo) {
    formatTime();
    logStream_ << T(logLevelStr[level_], 7);
}
Logger::Logger(LogLevel level) : level_(level) {
    formatTime();
    logStream_ << T(logLevelStr[level_], 7);
}
Logger::Logger(bool) : level_(kFatal) {
    formatTime();
    logStream_ << T(logLevelStr[level_], 7);
    if (errno != 0) {
        logStream_ << strerror_tl(errno) << " (errno=" << errno << ") ";
    }
}

RawLogger::~RawLogger() {
    if (index_ < 0) {
        auto& oFunc = Logger::outputFunc_();
        if (!oFunc)
            return;
        oFunc(logStream_.bufferData(), logStream_.bufferLength());
    } else {
        auto& oFunc = Logger::outputFunc_(index_);
        if (!oFunc)
            return;
        oFunc(logStream_.bufferData(), logStream_.bufferLength());
    }
}
Logger::~Logger() {
    if (sourceFile_.data_)
        logStream_ << T(" - ", 3) << sourceFile_ << ':' << fileLine_ << '\n';
    else
        logStream_ << '\n';
    if (index_ < 0) {
        auto& oFunc = Logger::outputFunc_();
        if (!oFunc)
            return;
        oFunc(logStream_.bufferData(), logStream_.bufferLength());
        if (level_ >= kError)
            Logger::flushFunc_()();
    } else {
        auto& oFunc = Logger::outputFunc_(index_);
        if (!oFunc)
            return;
        oFunc(logStream_.bufferData(), logStream_.bufferLength());
        if (level_ >= kError)
            Logger::flushFunc_(index_)();
    }
}
LogStream& Logger::stream() {
    return logStream_;
}
