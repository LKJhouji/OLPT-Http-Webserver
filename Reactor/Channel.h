#pragma once

#include <functional>
#include <sys/epoll.h>
#include <memory>

#include "noncopyable.h"
#include "Timestamp.h"

class EventLoop;  

//Channel支持监听读事件（acceptChannel的监听新连接（回调找轮询EventLoop创建HttpConnection连接）、监听HttpConnection新连接），写事件，更新事件，错误事件
//包括acceptChannel和Poller中的HttpConnectionChannel
class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    Channel(EventLoop* loop, int fd) : loop_(loop), fd_(fd), events_(0), status_(0), tied_(false) {}
    ~Channel() = default;

    int fd() const { return fd_; }
    EventLoop* loop() const { return loop_; }
    int status() const { return status_; }
    int events() const { return events_; }
    bool isReading() const { return events_ & kReadEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    void enableReading() { events_ |= (kReadEvent | kCloseEvent); update(); }
    void enableWriting() { events_ |= (kWriteEvent | kCloseEvent); update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }
    void setReadCallback(const EventCallback& cb) { readCallback_ = cb; }
    void setWriteCallback(const EventCallback& cb) { writeCallback_ = cb; }
    void setErrorCallback(const EventCallback& cb) { errorCallback_ = cb; }
    void setUpdateCallback(const EventCallback& cb) { updateCallback_ = cb; }
    void setCloseCallback(const EventCallback& cb) { closeCallback_ = cb; }
    void remove();
    void set_revents(int revents) { revents_ = revents; }
    void set_status(int status) { status_ = status; }
    void handleEvent();
    void tie(const std::shared_ptr<void>& obj); //是否被绑定到HttpConnection上了
private:
    void handleEventWithGuard();
    void update();
    EventLoop* loop_;
    int fd_;
    int events_;
    int revents_;
    int status_;
    std::shared_ptr<void> tie_; // 绑定到HttpConnection
    bool tied_;
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;
    static const int kCloseEvent;
    EventCallback writeCallback_;
    EventCallback errorCallback_;
    EventCallback readCallback_;
    EventCallback closeCallback_;
};