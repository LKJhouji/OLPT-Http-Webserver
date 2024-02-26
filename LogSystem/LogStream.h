#pragma once 
#include <string.h>
#include <iostream>
#include "noncopyable.h"

const int kSmallBuffer = 4000;      //Buffer的容量
const int kLargeBuffer = 4000*1000;

template<int SIZE>
class FixedBuffer : noncopyable {
public:
    FixedBuffer() : cur_(data_) {};
    ~FixedBuffer();

    void append(const char* buf, size_t len) {
        if (implicit_cast<size_t>(avail()) > len) {
            memcpy(cur_, buf, len);
            cur_ += len;
        }
    }
    void reset() { cur_ = data_; }
    int avail() const { return static_cast<int>(end() - cur_); }
    void bzero() { memZero(data_, sizeof data_); }
    char* current() { return cur_; }
    const char* data() const { return data_; }
    int length() const { return static_cast<int>(cur_ - data_); }
    void add(size_t len) { cur_ += len; }
private:
    const char* end() const { return data_ + sizeof data_; }
    char data_[SIZE];
    char *cur_;
};

class LogStream : noncopyable {
    using self = LogStream;
public:
    using Buffer = FixedBuffer<kSmallBuffer>;
    self& operator<< (bool v) {
        buffer_.append(v ? "1" : "0", 1);
        return *this;
    }
    self& operator<<(short);
    self& operator<<(unsigned short);
    self& operator<<(int);
    self& operator<<(unsigned int);
    self& operator<<(long);
    self& operator<<(unsigned long);
    self& operator<<(long long);
    self& operator<<(unsigned long long);
    self& operator<<(float v) {
        *this << static_cast<double>(v);
        return *this;
    }
    self& operator<<(double);
    self& operator<<(long double);
    self& operator<<(char v) {
        buffer_.append(&v, 1);
        return *this;
    }
    self& operator<<(const char* str) {
        if (str) buffer_.append(str, strlen(str));
        else buffer_.append("(null)", 6);
        return *this;
    }
    self& operator<<(const unsigned char* str) {
        return operator<<(reinterpret_cast<const char*>(str));
    }
    self& operator<<(const std::string& v) {
        buffer_.append(v.c_str(), v.size());
        return *this;
    }
    void append(const char* data, int len) { buffer_.append(data, static_cast<size_t>(len)); }
    const Buffer& buffer() const { return buffer_; }
    void resetBuffer() { buffer_.reset(); }

private:
    template<typename T>
        void formatInteger(T);
    Buffer buffer_;
    static const int kMaxNumericSize = 32;  //最大数字长度
};


