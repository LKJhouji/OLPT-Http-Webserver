#pragma once

//#include <fcntl.h>
#include <iostream>
#include <mutex>
#include "noncopyable.h"

//磁盘文件
class LogFile : noncopyable {
public:
    LogFile(const std::string& basename, std::string filepath) 
    : basename_(basename),  filepath_(filepath) {
        fp_ = fopen(filepath_.c_str(), "w+");  
    }
    ~LogFile() {
        fclose(fp_);
    }

    void append(const char* logline, int len);  //真正往磁盘里写东西
    void flush();
private:
    const std::string basename_;
    std::string filepath_;
    std::mutex mutex_;
    FILE* fp_;
};
