#pragma once
#include <iostream>
#include <memory>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdarg.h>
#include <fcntl.h>  
#include <mysql/mysql.h>
#include "Timer.h"
#include "LFUCache.h"
#include "noncopyable.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"
#include "SqlConnectionPool.h"


//#define ll long long
using namespace Lfu;
using std::string;
using std::shared_ptr;
using std::vector;
//study from qinguoyi's TinyWebserver
class HttpConnection : noncopyable, public std::enable_shared_from_this<HttpConnection> {
public:
    HttpConnection(int sockfd, 
            const InetAddress& localAddr, 
            string username, 
            string password, 
            string databaseName, 
            shared_ptr<EventLoop> loop,
            char* root);
    ~HttpConnection();
    
    void init();
    void closeConn();   //handleClose
    void process();     
    bool read();    //handleRead
    bool write(int iovcnt); //handleWrite
    void linkTimer(shared_ptr<Timer> timer) { timer_ = timer; }
private:
    enum METHOD { //http请求方法 
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum HTTP_CODE {   //http状态码
        NO_REQUEST,     //请求不完整，需要继续读取请求报文数据
        GET_REQUEST,    //获得了完整的HTTP请求
        BAD_REQUEST,    //HTTP请求报文有语法错误
        NO_RESOURCE,    //请求资源不存在
        FORBIDDEN_REQUEST,  //请求资源禁止访问，没有读取权限
        FILE_REQUEST,   //请求资源可以正常访问
        INTERNAL_ERROR, //服务器内部错误，该结果在主状态机逻辑switch的default下，一般不会触发
        CLOSED_CONNECTION
    };
    enum CHECK_STATE    //主状态机状态
    {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum LINE_STATUS {  //从状态机状态
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };
    void unmap();
    HTTP_CODE processRead();
    bool processWrite(HTTP_CODE ret);
    HTTP_CODE parseRequestLine(char *text);
    HTTP_CODE parseHeaders(char *text);
    HTTP_CODE parseContent(char *text);
    HTTP_CODE doRequest();
    char* getLine() { return startLine_; };
    LINE_STATUS parseLine();
    bool addResponse(const char *format, ...);
    bool addContent(const char *content);
    bool addStatusLine(int status, const char *title);
    bool addHeaders(int contentLength);
    bool addContentType();
    bool addContentLength(int contentLength);
    bool addLinger();
    bool addBlankLine();
    static const int fileNameLen_ = 200;
    static const int readBufferSize_ = 2048;
    static const int writeBufferSize_ = 1024;
    
    //std::unordered_map<string, string> users;   //跟业务相关
    shared_ptr<EventLoop> loop_;
    Socket socket_;
    Channel channel_;
    const InetAddress localAddr_;
    const InetAddress peerAddr_;
    string username_;
    string password_;
    string databaseName_;
    shared_ptr<MYSQL> mysql_;
    shared_ptr<SqlConnectionPool> sqlPool_;
    shared_ptr<LFUCache> cache_;
    shared_ptr<Timer> timer_;
    char* docRoot_;     //资源根目录
    Buffer inputBuffer_;  // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
    char* startLine_;
    CHECK_STATE checkState_;
    METHOD method_;
    string realFile_;
    int bytesToSend_;
    int bytesHaveSend_;
    struct stat fileStat_;  //refer to disk file
    string fileAddress_;
    struct iovec writeVec_[2];
    int writeIovcnt_;
    bool linger_;
    char* url_;
    char* version_;
    long ContentLength_;
    char* host_;
    char* string_;  //存储请求头数据
    int cgi_;    //是否启用的post
};