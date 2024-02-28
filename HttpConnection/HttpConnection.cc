#include "HttpConnection.h"
#include "SqlConnectionPool.h"

HttpConnection::HttpConnection(int sockfd, 
                const InetAddress& localAddr, 
                string username, 
                string password, 
                string databaseName,
                shared_ptr<EventLoop> loop,
                char* root) 
                : socket_(sockfd)
                , localAddr_(localAddr)
                , username_(username)
                , password_(password)
                , databaseName_(databaseName)
                , loop_(loop)
                , channel_(loop_.get(), sockfd)
                , docRoot_(root)
                , inputBuffer_(readBufferSize_)
                , outputBuffer_(writeBufferSize_)

{
    channel_.enableReading();
    channel_.enableWriting();
}
void HttpConnection::init() {
    mysql_ = nullptr;
    bytesToSend_ = bytesHaveSend_ = 0;
    inputBuffer_.retrieveAll();
    outputBuffer_.retrieveAll();
    checkState_ = CHECK_STATE_REQUESTLINE;  //check requestline
    method_ = GET;  //get
    startLine_ = 0;
}

// void HttpConnection::initSqlResult(SqlConnectionPool& scp) {
//     unique_ptr<MYSQL> conn = std::move(scp.getConnection());
//     MYSQL* mysql = conn.get();
//     if (mysql_query(mysql, "SELECT username, password FROM user") != 0) {
//         //TODO:LOG
//     }
//     MYSQL_RES* result = mysql_store_result(mysql);

//     while (MYSQL_ROW row = mysql_fetch_row(result)) {
//         users[row[0]] = row[1];
//     }
// }

void HttpConnection::closeConn() {
    loop_->removeChannel(&channel_);
}

bool HttpConnection::read() {
    long bytesRead = 0;

    //LT
    int savedErrno;
    bytesRead = inputBuffer_.readFd(socket_.fd(), &savedErrno);
    if (bytesRead <= 0) {
        //TODO : savedErrno
        return false;
    }
    return true;
}

bool HttpConnection::write() {
    int tmp = 0;
    if (bytesToSend_ == 0) {
        channel_.enableReading();   //write success, wait to read
        init();
        return true;
    }

    while (1) {
        int savedErrno;
        tmp = outputBuffer_.writeFd(socket_.fd(), &savedErrno);
        if (tmp < 0) {
            if (errno == EAGAIN) {
                channel_.enableWriting();
                return true;
            }
            unmap();
            return false;
        }

        bytesToSend_ -= tmp;
        bytesHaveSend_ += tmp;

        if (bytesToSend_ <= 0) {
            unmap();
            channel_.enableReading();

            return true;
        }

    }
}

void HttpConnection::unmap() {
    if (fileAddress_) {
        munmap(fileAddress_, fileStat_.st_size);   
        fileAddress_ = nullptr;
    }
}


void HttpConnection::process() {
    HTTP_CODE readRet = processRead();
    if (readRet == NO_REQUEST) {
        channel_.enableReading();
        return;
    }
    bool writeRet = processWrite(readRet);
    if (!writeRet) {
        //TODO
        closeConn();
    }
    channel_.enableWriting();
}

HttpConnection::HTTP_CODE HttpConnection::processRead() {
    LINE_STATUS lineStatus = LINE_OK;
    HTTP_CODE ret = NO_REQUEST;
    char* text = nullptr;
    
}
bool HttpConnection::processWrite(HTTP_CODE ret) {

}
