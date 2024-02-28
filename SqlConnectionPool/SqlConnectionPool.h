#pragma once

#include <mysql/mysql.h>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include "locker.h"
#include "noncopyable.h"
#include "InetAddress.h"

using std::string;
using std::unique_ptr;

//connection pool have many connections
//init pool, create many connections
//get connection
//release connection
//destroy connection
class SqlConnectionPool : noncopyable {
public:
    unique_ptr<MYSQL> getConnection();
    bool releaseConnection(unique_ptr<MYSQL> connection);
    int getFreeConn() { return FreeConn_; }
    void destroySqlConnectionPool();
    static SqlConnectionPool& getInstance();
    void init(InetAddress localAddr, string username, string password, string dataBaseName, int MaxConn);
private:
    SqlConnectionPool();
    ~SqlConnectionPool();
    int MaxConn_;
    int CurConn_;   //现在有多少已使用连接
    int FreeConn_;  //还有多少剩余连接
    std::list<unique_ptr<MYSQL>> connList_;    //RAII机制
    InetAddress localAddr_;
    string username_;
    string password_;
    string databaseName_;
    sem sem_;
    std::mutex mutex_;
};