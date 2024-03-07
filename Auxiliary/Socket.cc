#include "Socket.h"

Socket::~Socket() {
    ::close(sockfd_);
}

void Socket::listen() {
    if(::listen(sockfd_, 1024) == -1) {
        //LOG
    }
}

void Socket::bind(const InetAddress& localAddr) {
    if (::bind(sockfd_, (sockaddr*)localAddr.getSockAddr(), sizeof(sockaddr)) != 0) {
        //LOG
    }
}

int Socket::accept(InetAddress& peerAddr) {
    sockaddr_in sa;
    bzero(&sa, sizeof sa);     //sa 得置零，否则在新连接的时候会报错
    socklen_t st = sizeof(sa);
    int connfd = ::accept4(sockfd_, (sockaddr*)&sa, &st, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd >= 0) {
        peerAddr.setSockAddr(sa);
    }
    return connfd;
}

void Socket::setTcpNoDelay(bool on) {
    int flag = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_IP, TCP_NODELAY, &flag, sizeof flag);
}

void Socket::setReuseAddr(bool on) {
    int flag = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof flag);
}

void Socket::setReusePort(bool on) {
    int flag = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof flag);
}

void Socket::setKeepAlive(bool on) {
    int flag = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &flag, sizeof flag);
}
