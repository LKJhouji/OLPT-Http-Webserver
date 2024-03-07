#include "Channel.h"
#include "EventLoop.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI | EPOLLONESHOT; //后者表示紧急或周期性事件发生
const int Channel::kCloseEvent = EPOLLHUP | EPOLLRDHUP;
const int Channel::kWriteEvent = EPOLLOUT | EPOLLONESHOT;
void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::remove() {
    loop_->removeChannel(this);
}

void Channel::handleEventWithGuard() {
    //LOG
    //EPOLLHUP挂断
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        //读写双端都准备关闭连接，即服务器准备挂断
        if (closeCallback_) closeCallback_();
    }
    else if (revents_ & EPOLLERR) {
        if (errorCallback_) errorCallback_();
    }
    else if (revents_ & (EPOLLIN | EPOLLRDHUP | EPOLLPRI)) {
        //触发可读事件 | 对端关闭连接 | 高优先级可读
        if (readCallback_) readCallback_();
    }

    else if (revents_ & EPOLLOUT) {
        if (writeCallback_) writeCallback_();
    }
    
}

void Channel::handleEvent() {
    if (tied_) {
        if (tie_) {
            handleEventWithGuard();
        }
    }
    else {
        handleEventWithGuard();
    }
}

void Channel::tie(const std::shared_ptr<void>& obj)  {
    tie_ = obj;
    tied_ = true;
}