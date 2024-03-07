#include "Logger.h"

AsyncLogging* async = nullptr;

void asyncOutput(const char* logline, int len) {
    async->append(logline, len);
}

int main() {
    Logger::setOutput(std::bind(&asyncOutput, std::placeholders::_1, std::placeholders::_2));
    AsyncLogging log;
    async = &log;
    log.start();
    LOG << "Hello, World!";
    LOG << "LKJhouji";
    //log.stop();
    getchar();
    return 0;
}