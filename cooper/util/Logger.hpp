#ifndef log_Logger_hpp
#define log_Logger_hpp

#include <cstring>
#include <functional>
#include <iostream>
#include <vector>

#include "cooper/util/Date.hpp"
#include "cooper/util/LogStream.hpp"
#include "cooper/util/NonCopyable.hpp"

#define cooper_IF_(cond) for (int _r = 0; _r == 0 && (cond); _r = 1)

namespace cooper {
/**
 * @brief This class implements log functions.
 *
 */
class Logger : public NonCopyable {
public:
    enum LogLevel { kTrace = 0, kDebug, kInfo, kWarn, kError, kFatal, kNumberOfLogLevels };

    /**
     * @brief Calculate of basename of source files in compile time.
     *
     */
    class SourceFile {
    public:
        template <int N>
        inline SourceFile(const char (&arr)[N]) : data_(arr), size_(N - 1) {
            // std::cout<<data_<<std::endl;
            const char* slash = strrchr(data_, '/');  // builtin function
            if (slash) {
                data_ = slash + 1;
                size_ -= static_cast<int>(data_ - arr);
            }
        }

        explicit SourceFile(const char* filename = nullptr) : data_(filename) {
            if (!filename) {
                size_ = 0;
                return;
            }
            const char* slash = strrchr(filename, '/');
            if (slash) {
                data_ = slash + 1;
            }
            size_ = static_cast<int>(strlen(data_));
        }

        const char* data_;
        int size_;
    };
    Logger(SourceFile file, int line);
    Logger(SourceFile file, int line, LogLevel level);
    Logger(SourceFile file, int line, bool isSysErr);
    Logger(SourceFile file, int line, LogLevel level, const char* func);

    // LOG_COMPACT only <time><ThreadID><Level>
    Logger();
    Logger(LogLevel level);
    Logger(bool isSysErr);

    ~Logger();
    Logger& setIndex(int index) {
        index_ = index;
        return *this;
    }
    LogStream& stream();

    /**
     * @brief Set the output function.
     *
     * @param outputFunc The function to output a log message.
     * @param flushFunc The function to flush.
     * @note Logs are output to the standard output by default.
     */
    static void setOutputFunction(std::function<void(const char* msg, const uint64_t len)> outputFunc,
                                  std::function<void()> flushFunc, int index = -1) {
        if (index < 0) {
            outputFunc_() = outputFunc;
            flushFunc_() = flushFunc;
        } else {
            outputFunc_(index) = outputFunc;
            flushFunc_(index) = flushFunc;
        }
    }

    /**
     * @brief Set the log level. Logs below the level are not printed.
     *
     * @param level
     */
    static void setLogLevel(LogLevel level) {
        logLevel_() = level;
    }

    /**
     * @brief Get the current log level.
     *
     * @return LogLevel
     */
    static LogLevel logLevel() {
        return logLevel_();
    }

    /**
     * @brief Check whether it shows local time or UTC time.
     */
    static bool displayLocalTime() {
        return displayLocalTime_();
    }

    /**
     * @brief Set whether it shows local time or UTC time. the default is UTC.
     */
    static void setDisplayLocalTime(bool showLocalTime) {
        displayLocalTime_() = showLocalTime;
    }

protected:
    static void defaultOutputFunction(const char* msg, const uint64_t len) {
        fwrite(msg, 1, static_cast<size_t>(len), stdout);
    }
    static void defaultFlushFunction() {
        fflush(stdout);
    }
    void formatTime();
    static bool& displayLocalTime_() {
        static bool showLocalTime = false;
        return showLocalTime;
    }

    static LogLevel& logLevel_() {
#ifdef RELEASE
        static LogLevel logLevel = LogLevel::kInfo;
#else
        static LogLevel logLevel = LogLevel::kDebug;
#endif
        return logLevel;
    }
    static std::function<void(const char* msg, const uint64_t len)>& outputFunc_() {
        static std::function<void(const char* msg, const uint64_t len)> outputFunc = Logger::defaultOutputFunction;
        return outputFunc;
    }
    static std::function<void()>& flushFunc_() {
        static std::function<void()> flushFunc = Logger::defaultFlushFunction;
        return flushFunc;
    }
    static std::function<void(const char* msg, const uint64_t len)>& outputFunc_(size_t index) {
        static std::vector<std::function<void(const char* msg, const uint64_t len)>> outputFuncs;
        if (index < outputFuncs.size()) {
            return outputFuncs[index];
        }
        while (index >= outputFuncs.size()) {
            outputFuncs.emplace_back(outputFunc_());
        }
        return outputFuncs[index];
    }
    static std::function<void()>& flushFunc_(size_t index) {
        static std::vector<std::function<void()>> flushFuncs;
        if (index < flushFuncs.size()) {
            return flushFuncs[index];
        }
        while (index >= flushFuncs.size()) {
            flushFuncs.emplace_back(flushFunc_());
        }
        return flushFuncs[index];
    }
    friend class RawLogger;
    LogStream logStream_;
    Date date_{Date::now()};
    SourceFile sourceFile_;
    int fileLine_;
    LogLevel level_;
    int index_{-1};
};
class RawLogger : public NonCopyable {
public:
    ~RawLogger();
    RawLogger& setIndex(int index) {
        index_ = index;
        return *this;
    }
    LogStream& stream() {
        return logStream_;
    }

private:
    LogStream logStream_;
    int index_{-1};
};
#ifdef NDEBUG
#define LOG_TRACE cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kTrace, __func__).stream()
#else
#define LOG_TRACE                                                            \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kTrace)         \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kTrace, __func__) \
            .stream()
#define LOG_TRACE_TO(index)                                                  \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kTrace)         \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kTrace, __func__) \
            .setIndex(index)                                                 \
            .stream()

#endif

#define LOG_DEBUG                                                            \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kDebug)         \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kDebug, __func__) \
            .stream()
#define LOG_DEBUG_TO(index)                                                  \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kDebug)         \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kDebug, __func__) \
            .setIndex(index)                                                 \
            .stream()
#define LOG_INFO \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kInfo) cooper::Logger(__FILE__, __LINE__).stream()
#define LOG_INFO_TO(index)                                                                             \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kInfo) cooper::Logger(__FILE__, __LINE__) \
        .setIndex(index)                                                                               \
        .stream()
#define LOG_WARN cooper::Logger(__FILE__, __LINE__, cooper::Logger::kWarn).stream()
#define LOG_WARN_TO(index) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kWarn).setIndex(index).stream()
#define LOG_ERROR cooper::Logger(__FILE__, __LINE__, cooper::Logger::kError).stream()
#define LOG_ERROR_TO(index) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kError).setIndex(index).stream()
#define LOG_FATAL cooper::Logger(__FILE__, __LINE__, cooper::Logger::kFatal).stream()
#define LOG_FATAL_TO(index) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kFatal).setIndex(index).stream()
#define LOG_SYSERR cooper::Logger(__FILE__, __LINE__, true).stream()
#define LOG_SYSERR_TO(index) cooper::Logger(__FILE__, __LINE__, true).setIndex(index).stream()

// LOG_COMPACT_... begin block
#define LOG_COMPACT_DEBUG \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kDebug) cooper::Logger(cooper::Logger::kDebug).stream()
#define LOG_COMPACT_DEBUG_TO(index)                                                                         \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kDebug) cooper::Logger(cooper::Logger::kDebug) \
        .setIndex(index)                                                                                    \
        .stream()
#define LOG_COMPACT_INFO cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kInfo) cooper::Logger().stream()
#define LOG_COMPACT_INFO_TO(index) \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kInfo) cooper::Logger().setIndex(index).stream()
#define LOG_COMPACT_WARN cooper::Logger(cooper::Logger::kWarn).stream()
#define LOG_COMPACT_WARN_TO(index) cooper::Logger(cooper::Logger::kWarn).setIndex(index).stream()
#define LOG_COMPACT_ERROR cooper::Logger(cooper::Logger::kError).stream()
#define LOG_COMPACT_ERROR_TO(index) cooper::Logger(cooper::Logger::kError).setIndex(index).stream()
#define LOG_COMPACT_FATAL cooper::Logger(cooper::Logger::kFatal).stream()
#define LOG_COMPACT_FATAL_TO(index) cooper::Logger(cooper::Logger::kFatal).setIndex(index).stream()
#define LOG_COMPACT_SYSERR cooper::Logger(true).stream()
#define LOG_COMPACT_SYSERR_TO(index) cooper::Logger(true).setIndex(index).stream()
// LOG_COMPACT_... end block

#define LOG_RAW cooper::RawLogger().stream()
#define LOG_RAW_TO(index) cooper::RawLogger().setIndex(index).stream()

#define LOG_TRACE_IF(cond)                                                       \
    cooper_IF_((cooper::Logger::logLevel() <= cooper::Logger::kTrace) && (cond)) \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kTrace, __func__)     \
            .stream()
#define LOG_DEBUG_IF(cond)                                                       \
    cooper_IF_((cooper::Logger::logLevel() <= cooper::Logger::kDebug) && (cond)) \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kDebug, __func__)     \
            .stream()
#define LOG_INFO_IF(cond)                                                                                          \
    cooper_IF_((cooper::Logger::logLevel() <= cooper::Logger::kInfo) && (cond)) cooper::Logger(__FILE__, __LINE__) \
        .stream()
#define LOG_WARN_IF(cond) cooper_IF_(cond) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kWarn).stream()
#define LOG_ERROR_IF(cond) cooper_IF_(cond) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kError).stream()
#define LOG_FATAL_IF(cond) cooper_IF_(cond) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kFatal).stream()

#ifdef NDEBUG
#define DLOG_TRACE cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kTrace, __func__).stream()
#define DLOG_DEBUG cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kDebug, __func__).stream()
#define DLOG_INFO cooper_IF_(0) cooper::Logger(__FILE__, __LINE__).stream()
#define DLOG_WARN cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kWarn).stream()
#define DLOG_ERROR cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kError).stream()
#define DLOG_FATAL cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kFatal).stream()

#define DLOG_TRACE_IF(cond) cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kTrace, __func__).stream()
#define DLOG_DEBUG_IF(cond) cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kDebug, __func__).stream()
#define DLOG_INFO_IF(cond) cooper_IF_(0) cooper::Logger(__FILE__, __LINE__).stream()
#define DLOG_WARN_IF(cond) cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kWarn).stream()
#define DLOG_ERROR_IF(cond) cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kError).stream()
#define DLOG_FATAL_IF(cond) cooper_IF_(0) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kFatal).stream()
#else
#define DLOG_TRACE                                                           \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kTrace)         \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kTrace, __func__) \
            .stream()
#define DLOG_DEBUG                                                           \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kDebug)         \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kDebug, __func__) \
            .stream()
#define DLOG_INFO \
    cooper_IF_(cooper::Logger::logLevel() <= cooper::Logger::kInfo) cooper::Logger(__FILE__, __LINE__).stream()
#define DLOG_WARN cooper::Logger(__FILE__, __LINE__, cooper::Logger::kWarn).stream()
#define DLOG_ERROR cooper::Logger(__FILE__, __LINE__, cooper::Logger::kError).stream()
#define DLOG_FATAL cooper::Logger(__FILE__, __LINE__, cooper::Logger::kFatal).stream()

#define DLOG_TRACE_IF(cond)                                                      \
    cooper_IF_((cooper::Logger::logLevel() <= cooper::Logger::kTrace) && (cond)) \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kTrace, __func__)     \
            .stream()
#define DLOG_DEBUG_IF(cond)                                                      \
    cooper_IF_((cooper::Logger::logLevel() <= cooper::Logger::kDebug) && (cond)) \
        cooper::Logger(__FILE__, __LINE__, cooper::Logger::kDebug, __func__)     \
            .stream()
#define DLOG_INFO_IF(cond)                                                                                         \
    cooper_IF_((cooper::Logger::logLevel() <= cooper::Logger::kInfo) && (cond)) cooper::Logger(__FILE__, __LINE__) \
        .stream()
#define DLOG_WARN_IF(cond) cooper_IF_(cond) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kWarn).stream()
#define DLOG_ERROR_IF(cond) cooper_IF_(cond) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kError).stream()
#define DLOG_FATAL_IF(cond) cooper_IF_(cond) cooper::Logger(__FILE__, __LINE__, cooper::Logger::kFatal).stream()
#endif

const char* strerror_tl(int savedErrno);
}  // namespace cooper

#endif
