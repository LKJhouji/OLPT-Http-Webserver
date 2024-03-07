#pragma once

#include <iostream>
#include <memory>
#include <queue>

using std::shared_ptr;

class HttpConnection;

class Timer {
public:
    Timer(shared_ptr<HttpConnection> conn, int timeout);
    ~Timer();
    Timer(Timer& tm);
    void update(int timeout);  //update timer content
    bool isExpired();
    bool isDeleted() const { return deleted_; }
    void setExpired() { expired_ = true; }
    void setDeleted() { deleted_ = true; }
    size_t getExpiredTime() const { return expiredTime_; }
    void clear();
private:
    bool expired_;
    bool deleted_;
    int64_t expiredTime_;
    shared_ptr<HttpConnection> conn_;
};

struct TimerCmp {
    bool operator()(shared_ptr<Timer>& a, shared_ptr<Timer>& b) {
        return a->getExpiredTime() > b->getExpiredTime();
    }
};

class TimerManager {
public:
    TimerManager();
    ~TimerManager();
    void addTimer(shared_ptr<HttpConnection>& conn, int timeout);
    void handleExpiredEvent();
private:
    using timerPtr = shared_ptr<Timer>;
    std::priority_queue<timerPtr, std::deque<timerPtr>, TimerCmp> timerQueue_;

};