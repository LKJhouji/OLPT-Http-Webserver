#pragma once

#include <iostream>
#include <vector>
#include <memory>
#include <condition_variable>
#include "noncopyable.h"
#include "LogStream.h"
#include "Thread.h"
#include "CountDownLatch.h"
#include "LogFile.h"

class LogFile;
class AsyncLogging : noncopyable {
public:
    AsyncLogging(const std::string basename = "async logging", int flushInterval = 3);
    ~AsyncLogging() {
        if (running_) stop();
    }

    void append(const char* logline, int len);
    void start();
    void stop();
    
private:
    using Buffer = FixedBuffer<kLargeBuffer>;
    using BufferVector = std::vector<std::shared_ptr<Buffer>>;
    using BufferPtr = std::shared_ptr<Buffer>;

    void threadFunc();  //后端日志线程函数，用于把缓冲区日志写入文件
    const int flushInterval_;    //超时时间，每隔一段时间写日志
    bool running_;
    const std::string basename_;    //日志名字
    Thread thread_; //后端线程
    std::mutex mutex_;
    std::condition_variable cond_;
    BufferPtr currentBuffer_;   //相当于Buffer B
    BufferPtr nextBuffer_;
    BufferVector buffers_;  //后端缓冲区队列
    CountDownLatch latch_;  //倒计时，用于指示日志记录器何时开始工作
};
