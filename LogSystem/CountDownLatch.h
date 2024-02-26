#pragma once

#include <mutex>
#include <condition_variable>

#include "noncopyable.h"

class CountDownLatch : noncopyable {
public:
    explicit CountDownLatch(int count);
    void wait();
    void countDown();
    int getCount() const { return count_; }
private:
    std::mutex mutex_;
    std::condition_variable cond_;
    int count_;
};