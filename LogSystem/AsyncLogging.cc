#include <assert.h>
#include "AsyncLogging.h"
#include "MemoryPool.h"

using namespace MPool;

AsyncLogging::AsyncLogging(const std::string basename, 
                            int flushInterval) 
                            : flushInterval_(flushInterval)
                            , running_(false)
                            , basename_(basename)
                            , thread_(std::bind(&AsyncLogging::threadFunc, this), "Logging")
                            , mutex_()
                            , cond_()
                            , currentBuffer_(newElement<Buffer>())
                            , nextBuffer_(newElement<Buffer>())
                            , buffers_()
                            , latch_(1) {
    currentBuffer_->bzero();
    nextBuffer_->bzero();
    buffers_.reserve(16);
}

void AsyncLogging::append(const char* logline, int len) {
    std::unique_lock<std::mutex> lk(mutex_);
    if (currentBuffer_->avail() > len) {
        currentBuffer_->append(logline, len);
    }
    else {
        buffers_.push_back(std::move(currentBuffer_));
        if (nextBuffer_) {
            currentBuffer_ = std::move(nextBuffer_);
        }
        else {
            currentBuffer_.reset(newElement<Buffer>()); //使用的是shared_ptr的reset函数，指向新的new Buffer，这种情况很少发生
        }
        currentBuffer_->append(logline, len);
        cond_.notify_one();
    }
}

void AsyncLogging::start() {
    running_ = true;
    thread_.start();
    latch_.wait();
}

void AsyncLogging::stop() {
    running_ = false;
    cond_.notify_one();
    thread_.join(); 
}

void AsyncLogging::threadFunc() {
    assert(running_ == true);

    latch_.countDown();
    LogFile output(basename_, "../LogOutput/LogFile.txt");
    BufferPtr newBuffer1(newElement<Buffer>());
    BufferPtr newBuffer2(newElement<Buffer>());
    newBuffer1->bzero();
    newBuffer2->bzero();
    BufferVector buffersToWrite;    //该vector属于后端线程，用于和前端buffers进行交换
    buffersToWrite.reserve(16);
    while (running_) {
        assert(newBuffer1 && newBuffer1->length() == 0);
        assert(newBuffer2 && newBuffer2->length() == 0);
        assert(buffersToWrite.empty());
        //将前端buffers_中数据交换到BuffersToWrite中
        {
            std::unique_lock<std::mutex> lk(mutex_);
            //每个3s，或者currentBuffer满了，就将currentBuffer放入buffers_中
            if (buffers_.empty())
                cond_.wait_for(lk, std::chrono::seconds(flushInterval_));

            buffers_.push_back(std::move(currentBuffer_));
            currentBuffer_ = std::move(newBuffer1);
            buffersToWrite.swap(buffers_);
            if (!nextBuffer_)
                nextBuffer_ = std::move(newBuffer2);
        }
        assert(!buffersToWrite.empty());

        //如果队列中buffer数目大于25，删除多余数据
        //避免日志堆积，前端日志记录太快，后端来不及写入文件
        if (buffersToWrite.size() > 25) {
            //TODO:删除时加错误提示
            //只留原始的两个buffer，其余的删除
            buffersToWrite.erase(buffersToWrite.begin() + 2, buffersToWrite.end());
        }
        for (const auto& buffer : buffersToWrite) {
            output.append(buffer->data(), buffer->length());
        }
        //重新调整buffersToWrite的大小，仅保留两个原始buffer
        if (buffersToWrite.size() > 2) {
            buffersToWrite.resize(2);
        }
        if (!newBuffer1) {
            assert(!buffersToWrite.empty());
            newBuffer1 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer1->reset();
        }
        if (!newBuffer2) {
            assert(!buffersToWrite.empty());
            newBuffer2 = buffersToWrite.back();
            buffersToWrite.pop_back();
            newBuffer2->reset();
        }
        buffersToWrite.clear();
        output.flush();
    }
    output.flush();
}