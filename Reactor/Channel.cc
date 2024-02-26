#include "Channel.h"
#include "EventLoop.h"
#include "Log.h"

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; //后者表示紧急或周期性事件发生
const int Channel::kWriteEvent = EPOLLOUT;
void Channel::update() {
    loop_->updateChannel(this);
}

void Channel::remove() {
    loop_->removeChannel(this);
}

void Channel::handleEventWithGuard() {
    LOG_INFO("%s--%s--%d : channel handle revents : %d in thread %d\n", __FILE__, __FUNCTION__, __LINE__, revents_, loop_->threadId());
    //EPOLLHUP挂断
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        //挂起事件
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
    
    if (updateCallback_) updateCallback_(); //触发更新监听事件
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