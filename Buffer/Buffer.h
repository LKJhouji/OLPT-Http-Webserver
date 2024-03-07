#pragma once 

#include <iostream>
#include <vector>
#include <sys/stat.h>
#include <stdarg.h>

using std::string;

class Buffer {
public:
    static const int kCheapPrepend = 8; //预留空间初始化大小为8
    static const int kInitialSize = 1024;
    explicit Buffer(size_t initialSize = kInitialSize) : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {}
    size_t readableBytes() const { return writerIndex_ - readerIndex_; }
    size_t writableBytes() const { return buffer_.size() - writerIndex_; }
    size_t prependableBytes() const { return readerIndex_; }
    void retrieve(size_t len) {
        if (len < readableBytes()) {
            readerIndex_ += len;
        } else {
            retrieveAll();
        }
    }
    void retrieveAll() {    //复位操作
        readerIndex_ = kCheapPrepend;
        writerIndex_ = kCheapPrepend;
    }
    std::string retrieveAllAsString() {
        return retrieveAsString(readableBytes());
    }
    std::string retrieveAsString(size_t len) {
        std::string result(begin() + readerIndex_, len);
        retrieve(len);
        return result;
    }
    void ensureWritableBytes(size_t len) {
        if (writableBytes() < len) {
            makeSpace(len);
        }
    }
    void append(const char* data, size_t len) {
        ensureWritableBytes(len);
        std::copy(data, data + len, begin() + writerIndex_);
        writerIndex_ += len;
    }
    bool vappend(size_t maxLen, const char* format, ...);
    ssize_t readFd(int fd, int* savedErrno);
    ssize_t writeFd(int fd, int* savedErrno, struct iovec* vec, int iovcnt, string fileAddress, int bytesHaveSend, int bytesToSend);
    char* peek() { return begin() + readerIndex_; }
    const char* peek() const { return begin() + readerIndex_; }
    char* beginWrite() { return begin() + writerIndex_; }
    const char* beginWrite() const { return begin() + writerIndex_; }
    const char* cheapPrepend() const { return begin() + kCheapPrepend; }
private:
    void makeSpace(size_t len) {
        if (writableBytes() + prependableBytes() < len + kCheapPrepend) {   //可写空间 + 预留空间 < 要写空间 + 初始化的预留空间
            buffer_.resize(writerIndex_ + len);
        }
        else {
            std::copy(&*buffer_.begin() + readerIndex_, &*buffer_.begin() + writerIndex_, &*buffer_.begin() + kCheapPrepend);
            readerIndex_ = kCheapPrepend;
            writerIndex_ = readerIndex_ + readableBytes();
        }
    }
    char* begin() { return &*buffer_.begin(); }

    const char* begin() const { return &*buffer_.begin(); }
    std::vector<char> buffer_;
    size_t readerIndex_;
    size_t writerIndex_;
};