#pragma once

//#include <fcntl.h>
#include <iostream>
#include <mutex>
#include "noncopyable.h"

//磁盘文件
class LogFile : noncopyable {
public:
    LogFile(const std::string& basename) 
    : basename_(basename) {}
    ~LogFile() = default;

    void append(const char* logline, int len);  //真正往磁盘里写东西
    void flush();
private:
    const std::string basename_;
    std::mutex mutex_;
    FILE* fp_;
};
