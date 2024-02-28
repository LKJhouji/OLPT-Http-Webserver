#pragma once

#include <memory>
#include <unordered_map>
#include <sys/stat.h>
#include <sys/mman.h>
#include "noncopyable.h"
#include "Socket.h"
#include "EventLoop.h"
#include "Buffer.h"
#include "Channel.h"

//#define ll long long

using std::string;
using std::unique_ptr;
using std::shared_ptr;
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
    void closeConn();
    void process();
    bool read();
    bool write();
    //void initSqlResult(SqlConnectionPool& scp); should be put into 'HttpWebserver' main class
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
    void handleRead();
    void handleWrite();
    void handleError();
    void handleUpdate();
    void unmap();
    HTTP_CODE processRead();
    bool processWrite(HTTP_CODE ret);
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
    unique_ptr<MYSQL> mysql_;
    
    char* docRoot_;     //资源根目录
    Buffer inputBuffer_;  // 接收数据的缓冲区
    Buffer outputBuffer_; // 发送数据的缓冲区
    long checkedIdx_;
    int startLine_;
    CHECK_STATE checkState_;
    METHOD method_;
    char realFile[fileNameLen_];
    int bytesToSend_;
    int bytesHaveSend_;
    struct stat fileStat_;  //refer to disk file
    char* fileAddress_;
};