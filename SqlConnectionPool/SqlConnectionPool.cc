#include "SqlConnectionPool.h"

shared_ptr<MYSQL> SqlConnectionPool::getConnection() {
    shared_ptr<MYSQL> conn = nullptr;
    if (0 == connList_.size()) return nullptr;
    sem_.wait();
    {
        std::unique_lock<std::mutex> lk(mutex_);
        conn = std::move(connList_.front());
        connList_.pop_front();
        FreeConn_--;
        CurConn_++;
    }
    return conn;
}

bool SqlConnectionPool::releaseConnection(shared_ptr<MYSQL> conn) {
    if (nullptr == conn) return false;
    {
        std::unique_lock<std::mutex> lk(mutex_);
        connList_.push_back(std::move(conn));
        FreeConn_++;
        CurConn_--;
    }
    return true;
}

void SqlConnectionPool::destroySqlConnectionPool() {
    {
        std::unique_lock<std::mutex> lk(mutex_);
        if (connList_.size() > 0) {
            std::list<shared_ptr<MYSQL>>::iterator it;
            for (it = connList_.begin(); it != connList_.end(); it++) {
                MYSQL* conn = it->get();
                mysql_close(conn);
            }
            CurConn_ = 0;
            FreeConn_ = 0;
            connList_.clear();
        }
    }
}

SqlConnectionPool& SqlConnectionPool::getInstance() {
    static SqlConnectionPool connPool;
    return connPool;
}

void SqlConnectionPool::init(InetAddress localAddr, string username, string password, string databaseName, int MaxConn) {
    localAddr_ = localAddr;
    username_ = username;
    password_ = password;
    databaseName_ = databaseName;
    MaxConn_ = MaxConn;
    
    for (int i = 0; i < MaxConn; i++) {
        MYSQL* conn = nullptr;
        conn = mysql_init(conn);
        if (!conn) {
            //TODO:LOG
            exit(-1);
        }
        conn = mysql_real_connect(conn, localAddr_.toIp().c_str(), username_.c_str(), password_.c_str(), databaseName_.c_str(), localAddr_.toPort(), NULL, 0);

        if (!conn) {
            //TODO:LOG
            exit(-1);
        }
        shared_ptr<MYSQL> p(conn);
        connList_.push_back(p);
        FreeConn_++;
    }
    Sem sem_ = Sem(FreeConn_);
}

SqlConnectionPool::SqlConnectionPool() : CurConn_(0), FreeConn_(0), isInited_(false) {
}

SqlConnectionPool::~SqlConnectionPool() {
    destroySqlConnectionPool();
}

void SqlConnectionPool::initSqlResult() {
    if (!isInited_) {
        shared_ptr<MYSQL> conn = getConnection();
        MYSQL* mysql = conn.get();
        if (mysql_query(mysql, "SELECT username, password FROM user") != 0) {
            //TODO:LOG
        }
        MYSQL_RES* result = mysql_store_result(mysql);

        while (MYSQL_ROW row = mysql_fetch_row(result)) {
            users_[row[0]] = row[1];
        }
        isInited_ = true;
    }
    
}