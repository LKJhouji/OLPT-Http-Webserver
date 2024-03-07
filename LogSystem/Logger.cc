#include "Logger.h"

Logger::OutputFunc Logger::output_ = 0;

Logger::Logger(const char* filename, int line) 
                : impl_(filename, line) {
    
}

Logger::~Logger() {
    impl_.stream_ << "  --  " << impl_.curTime_.now().toString() << ':' << impl_.basename_ << ':' << impl_.line_ << "\n";
    const LogStream::Buffer& buf(stream().buffer());
    Logger::output_(buf.data(), buf.length());
}

Logger::Impl::Impl(const char* fileName, int line) 
                    : basename_(fileName)
                    , line_(line) {}

void Logger::setOutput(OutputFunc output) { output_ = output; }
