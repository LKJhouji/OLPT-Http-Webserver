#include <sys/eventfd.h>
#include <mutex>
#include "EventLoop.h"
#include "Poller.h"
#include "Channel.h"
#include "MemoryPool.h"

using namespace MPool;

__thread EventLoop* t_loopInThisThread = nullptr; //线程中对应的EventLoop

const int kPollTimeMs = 30000; //poller的等待时间

int createEventFd() {
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0) {
        //LOG
    }
    return evtfd;
}

EventLoop::EventLoop() : threadId_(CurrentThread::tid()), poller_(Poller::newDefaultPoller(this)), looping_(false), quit_(false), callingPendingFunctors_(false), wakeupFd_(createEventFd()), wakeupChannel_(newElement<Channel>(this, wakeupFd_)) {
    //LOG
    if (t_loopInThisThread) {
        //LOG
    }
    else t_loopInThisThread = this;
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::loop() {
    looping_ = true;
    quit_ = false;
    //LOG
    while (!quit_) {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel* channel : activeChannels_) {
            channel->handleEvent();
        }
        doPendingFunctors();
    }
    //LOG
    looping_ = false;
}

void EventLoop::quit() {
    quit_ = true;
    
    if (!isInLoopThread()) {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb) {
    if (isInLoopThread()) {
        cb();
    }
    else {
        queueInLoop(cb);
    }
}

void EventLoop::queueInLoop(Functor cb) {
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.push_back(cb);
    }
    if (!isInLoopThread() || callingPendingFunctors_) {
        wakeup();
    }
}

void EventLoop::updateChannel(Channel* channel) {
    poller_->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    poller_->removeChannel(channel);
}

void EventLoop::wakeup() {
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        //LOG
    }
}

bool EventLoop::hasChannel(Channel* channel) {
    return poller_->hasChannel(channel);
}

void EventLoop::handleRead() {
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one) {
        //LOG
    }
}
void EventLoop::doPendingFunctors() {
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor &functor : functors)
    {
        functor(); // 执行当前loop需要执行的回调操作
    }
    
    if (functors.empty()) {
        //LOG
        return;
    }
    callingPendingFunctors_ = false;

    //LOG
}