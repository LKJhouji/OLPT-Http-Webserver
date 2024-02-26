#include "Logger.h"

Logger::Logger(const char* filename, int line) 
                : impl_(filename, line) 
                , asyncLog_() {
    setOutput(std::bind(&Logger::asyncOutput, this, std::placeholders::_1, std::placeholders::_2));
}

Logger::~Logger() {
    impl_.stream_ << "--" << impl_.curTime_.now().toString() << ':' << impl_.basename_ << ':' << impl_.line_ << "\n";
    const LogStream::Buffer& buf(stream().buffer());
    output_(buf.data(), buf.length());
}

Logger::Impl::Impl(const char* fileName, int line) 
                    : basename_(fileName)
                    , line_(line) {}
