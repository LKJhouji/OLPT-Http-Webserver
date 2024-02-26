#pragma once
#include <functional>
#include "LogStream.h"
#include "AsyncLogging.h"
#include "Timestamp.h"
#define LOG Logger(__FILE__, __LINE__).stream()

class Logger {
    using OutputFunc = std::function<void(const char* logline, int len)>;
public:
    enum LogLevel {
        INFO,
        WARN,
        ERROR,
        FATAL,
    };
    Logger(const char* filename, int line);
    ~Logger();
    LogStream& stream() { return impl_.stream_; }
    void setLogFileName(std::string fileName) { logFileName_ = fileName; }
    std::string getLogFileName() { return logFileName_; }
    LogLevel logLevel() { return logLevel_; }
    void setLogLevel(LogLevel level) { logLevel_ = level; }
    static void setOutput(OutputFunc output) { output_ = output; }
private:
    void asyncOutput(const char* logline, int len) {
        asyncLog_.append(logline, len);
    }
    class Impl {
    public:
        Impl(const char* fileName, int line);
        Timestamp curTime_;
        LogStream stream_;  //文件流对象，进行文件写入操作
        int line_;
        std::string basename_;
    };
    Impl impl_;
    std::string logFileName_;
    static OutputFunc output_;
    AsyncLogging asyncLog_;
    LogLevel logLevel_;
};