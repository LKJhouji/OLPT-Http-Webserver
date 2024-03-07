#include "HttpConnection.h"
#include "SqlConnectionPool.h"

const char *ok200Title = "OK";
const char *error400Title = "Bad Request";
const char *error400Form = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const char *error403Title = "Forbidden";
const char *error403Form = "You do not have permission to get file form this server.\n";
const char *error404Title = "Not Found";
const char *error404Form = "The requested file was not found on this server.\n";
const char *error500Title = "Internal Error";
const char *error500Form = "There was an unusual problem serving the request file.\n";




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
                , cache_(&getCache())
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
    startLine_ = nullptr;
}



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

bool HttpConnection::write(int iovcnt) {
    int tmp = 0;
    if (bytesToSend_ == 0) {
        channel_.enableReading();   //write success, wait to read
        init();
        return true;
    }

    while (1) {
        int savedErrno;
        tmp = outputBuffer_.writeFd(socket_.fd(), &savedErrno, writeVec_, writeIovcnt_, fileAddress_, bytesHaveSend_, bytesToSend_);
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
    if (!fileAddress_.empty()) {
        munmap(const_cast<char*>(fileAddress_.c_str()), fileStat_.st_size);   
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
    while ((checkState_ == CHECK_STATE_CONTENT && lineStatus == LINE_OK) || ((lineStatus = parseLine()) == LINE_OK))
    {
        text = getLine();
        startLine_ = outputBuffer_.peek();
        
        switch (checkState_) {
        case CHECK_STATE_REQUESTLINE: {
            ret = parseRequestLine(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            break;
        }
        case CHECK_STATE_HEADER: {
            ret = parseHeaders(text);
            if (ret == BAD_REQUEST)
                return BAD_REQUEST;
            else if (ret == GET_REQUEST) 
                return doRequest();
            break;
        }
        case CHECK_STATE_CONTENT: {
            ret = parseContent(text);
            if (ret == GET_REQUEST)
                return doRequest();
            lineStatus = LINE_OPEN; //方便跳出循环
            break;
        }
        default:
            return INTERNAL_ERROR;
        }
    }
    return NO_REQUEST;
}

bool HttpConnection::processWrite(HTTP_CODE ret)
{
    switch (ret) {
    case INTERNAL_ERROR: {
        addStatusLine(500, error500Title);
        addHeaders(strlen(error500Form));
        if (!addContent(error500Form))
            return false;
        break;
    }
    case BAD_REQUEST: {
        addStatusLine(404, error404Title);
        addHeaders(strlen(error400Form));
        if (!addContent(error400Form))
            return false;
        break;
    }
    case FORBIDDEN_REQUEST: {
        addStatusLine(403, error403Title);
        addHeaders(strlen(error403Form));
        if (!addContent(error403Form))
            return false;
        break;
    }
    case FILE_REQUEST: {
        addStatusLine(200, ok200Title);
        if (fileStat_.st_size != 0) {
            addHeaders(fileStat_.st_size);
            writeVec_[0].iov_base = outputBuffer_.peek();
            writeVec_[0].iov_len = outputBuffer_.readableBytes();
            writeVec_[1].iov_base = const_cast<char*>(fileAddress_.c_str());
            writeVec_[1].iov_len = fileStat_.st_size;
            writeIovcnt_ = 2;
            bytesToSend_ = outputBuffer_.readableBytes() + fileStat_.st_size;
            return true;
        }
        else {
            const char *okString = "<html><body></body></html>";
            addHeaders(strlen(okString));
            if (!addContent(okString))
                return false;
        }
    }
    default:
        return false;
    }
    writeVec_[0].iov_base = outputBuffer_.peek();
    writeVec_[0].iov_len = outputBuffer_.readableBytes();
    writeIovcnt_ = 1;
    bytesToSend_ = outputBuffer_.readableBytes();
    return true;
}

bool HttpConnection::addResponse(const char *format, ...) {
    return outputBuffer_.vappend(writeBufferSize_, format);
}
bool HttpConnection::addStatusLine(int status, const char *title) {
    return addResponse("%s %d %s\r\n", "HTTP/1.1", status, title);
}
bool HttpConnection::addHeaders(int contentLen) {
    return addContentLength(contentLen) && addLinger() &&
           addBlankLine();
}
bool HttpConnection::addContentLength(int contentLen) {
    return addResponse("Content-Length:%d\r\n", contentLen);
}
bool HttpConnection::addContentType() {
    return addResponse("Content-Type:%s\r\n", "text/html");
}
bool HttpConnection::addLinger() {
    return addResponse("Connection:%s\r\n", (linger_ == true) ? "keep-alive" : "close");
}
bool HttpConnection::addBlankLine() {
    return addResponse("%s", "\r\n");
}
bool HttpConnection::addContent(const char *content) {
    return addResponse("%s", content);
}

HttpConnection::LINE_STATUS HttpConnection::parseLine() {
    char temp;
    for (; inputBuffer_.readableBytes(); inputBuffer_.retrieve(1))
    {
        temp = *(inputBuffer_.peek());
        if (temp == '\r')
        {
            if (inputBuffer_.readableBytes() == 1)
                return LINE_OPEN;
            else if (*(inputBuffer_.peek() + 1) == '\n')
            {
                *(inputBuffer_.peek()) = '\0';
                inputBuffer_.retrieve(1);
                *(inputBuffer_.peek()) = '\0';
                inputBuffer_.retrieve(1);
                return LINE_OK;
            }
            return LINE_BAD;
        }
        else if (temp == '\n')
        {
            if ((inputBuffer_.peek() - inputBuffer_.cheapPrepend() > 1) && *(inputBuffer_.peek() - 1) == '\r')
            {
                *(inputBuffer_.peek() - 1) = '\0';
                *(inputBuffer_.peek()) = '\0';
                inputBuffer_.retrieve(1);
                return LINE_OK;
            }
            return LINE_BAD;
        }
    }
    return LINE_OPEN;
}

HttpConnection::HTTP_CODE HttpConnection::parseRequestLine(char *text) {
    url_ = strpbrk(text, " \t");
    if (!url_)
        return BAD_REQUEST;
    *url_++ = '\0';
    char *method = text;
    if (strcasecmp(method, "GET") == 0)
        method_ = GET;
    else if (strcasecmp(method, "POST") == 0)
        method_ = POST;
    else
        return BAD_REQUEST;
    url_ += strspn(url_, " \t");
    version_ = strpbrk(url_, " \t");
    if (!version_)
        return BAD_REQUEST;
    *version_++ = '\0';
    version_ += strspn(version_, " \t");
    if (strcasecmp(version_, "HTTP/1.1") != 0)
        return BAD_REQUEST;
    if (strncasecmp(url_, "http://", 7) == 0) {
        url_ += 7;
        url_ = strchr(url_, '/');
    }

    if (strncasecmp(url_, "https://", 8) == 0) {
        url_ += 8;
        url_ = strchr(url_, '/');
    }

    if (!url_ || url_[0] != '/')
        return BAD_REQUEST;
    //当url为/时，显示判断界面
    if (strlen(url_) == 1)
        strcat(url_, "judge.html");
    checkState_ = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

//解析http请求的一个头部信息
HttpConnection::HTTP_CODE HttpConnection::parseHeaders(char *text) {
    if (text[0] == '\0') {
        if (ContentLength_ != 0) {
            checkState_ = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    }
    else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            linger_ = true;
        }
    }
    else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        ContentLength_ = atol(text);
    }
    else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        host_ = text;
    }
    else {
        //LOG
    }
    return NO_REQUEST;
}

//判断http请求是否被完整读入
HttpConnection::HTTP_CODE HttpConnection::parseContent(char *text) {
    if (inputBuffer_.readableBytes() >= ContentLength_) {
        text[ContentLength_] = '\0';
        //POST请求中最后为输入的用户名和密码
        string_ = text;
        return GET_REQUEST;
    }
    return NO_REQUEST;
}

HttpConnection::HTTP_CODE HttpConnection::doRequest() {
    realFile_ += docRoot_;
    const char *p = strrchr(url_, '/');
    
    auto& users = sqlPool_->getUsersInfor();
    auto& mtx = sqlPool_->getMutex();
    //处理cgi
    if (cgi_ == 1 && (*(p + 1) == '2' || *(p + 1) == '3')) {

        //根据标志判断是登录检测还是注册检测
        char flag = url_[1];

        string urlReal;
        urlReal += "/";
        urlReal += (url_ + 2);
        realFile_ += urlReal;

        //将用户名和密码提取出来
        //user=123&passwd=123
        string name(200, 0), password(200, 0);
        int i;
        for (i = 5; string_[i] != '&'; ++i)
            name[i - 5] = string_[i];
        name[i - 5] = '\0';

        int j = 0;
        for (i = i + 10; string_[i] != '\0'; ++i, ++j)
            password[j] = string_[i];
        password[j] = '\0';

        if (*(p + 1) == '3') {
            //如果是注册，先检测数据库中是否有重名的
            //没有重名的，进行增加数据
            string sqlInsert;
            sqlInsert += "INSERT INTO user(username, passwd) VALUES(";
            sqlInsert += "'";
            sqlInsert += name;
            sqlInsert += "', '";
            sqlInsert += password;
            sqlInsert += "')";
            if (users.find(name) == users.end()) {
                int res;
                {
                    std::unique_lock<std::mutex> lk(mtx);
                    res = mysql_query(mysql_.get(), sqlInsert.c_str());
                    users.insert(std::pair<string, string>(name, password));
                }

                if (!res)
                    strcpy(url_, "/log.html");
                else
                    strcpy(url_, "/registerError.html");
            }
            else
                strcpy(url_, "/registerError.html");
        }
        //如果是登录，直接判断
        //若浏览器端输入的用户名和密码在表中可以查找到，返回1，否则返回0
        else if (*(p + 1) == '2') {
            if (users.find(name) != users.end() && users[name] == password)
                strcpy(url_, "/welcome.html");
            else
                strcpy(url_, "/logError.html");
        }
    }

    if (*(p + 1) == '0') {
        string urlReal;
        urlReal += "/register.html";
        realFile_ += urlReal;
    }
    else if (*(p + 1) == '1') {
        string urlReal;
        urlReal += "/log.html";
        realFile_ += urlReal;
    }
    else if (*(p + 1) == '5') {
        string urlReal;
        urlReal += "/picture.html";
        realFile_ += urlReal;
    }
    else if (*(p + 1) == '6') {
        string urlReal;
        urlReal += "/video.html";
        realFile_ += urlReal;
    }
    else if (*(p + 1) == '7') {
        string urlReal;
        urlReal += "/fans.html";
        realFile_ += urlReal;
    }
    else
        realFile_+= url_;

    if (stat(realFile_.c_str(), &fileStat_) < 0)
        return NO_RESOURCE;

    if (!(fileStat_.st_mode & S_IROTH))
        return FORBIDDEN_REQUEST;

    if (S_ISDIR(fileStat_.st_mode))
        return BAD_REQUEST;
        
    if (!cache_->get(realFile_, fileAddress_)) {
        int fd = open(realFile_.c_str(), O_RDONLY);
        fileAddress_ = (char *)mmap(0, fileStat_.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        close(fd);
        cache_->set(realFile_, fileAddress_);
    }
    return FILE_REQUEST;
}