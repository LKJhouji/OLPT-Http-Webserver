#include "Timer.h"
#include "HttpConnection.h"

Timer::Timer(shared_ptr<HttpConnection> conn, int timeout)
    : conn_(std::move(conn)), deleted_(false) {
        expiredTime_ = time(nullptr) + timeout;
}

Timer::~Timer() {
    if (conn_) conn_->closeConn();
}

Timer::Timer(Timer& tm) : conn_(std::move(tm.conn_)), expiredTime_(0) {}

void Timer::update(int timeout) { //update timer content
    expiredTime_ = time(nullptr) + timeout;
}  
bool Timer::isExpired() {
    if (time(NULL) < expiredTime_) return false;
    else {
        setDeleted();
        return true;
    }
}

void Timer::clear() {
    conn_.reset();
    setDeleted();
}

TimerManager::TimerManager() {

}
TimerManager::~TimerManager() {

}


void TimerManager::addTimer(shared_ptr<HttpConnection>& conn, int timeout) {
    timerPtr Node(new Timer(conn, timeout));
    timerQueue_.push(Node);

}
void TimerManager::handleExpiredEvent() {
    while (!timerQueue_.empty()) {
        timerPtr Node = timerQueue_.top();
        if (Node->isDeleted()) {
            timerQueue_.pop();
        }
        else if (Node->isExpired() == true) {
            timerQueue_.pop();
        }
        else break;
    }
}