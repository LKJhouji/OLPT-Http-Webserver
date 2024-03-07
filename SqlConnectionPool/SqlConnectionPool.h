#pragma once

#include <mysql/mysql.h>
#include <iostream>
#include <list>
#include <memory>
#include <mutex>
#include <unordered_map>
#include "Locker.h"
#include "noncopyable.h"
#include "InetAddress.h"

using std::string;
using std::shared_ptr;
using std::unordered_map;
using std::mutex;
//connection pool have many connections
//init pool, create many connections
//get connection
//release connection
//destroy connection
class SqlConnectionPool : noncopyable {
public:
    shared_ptr<MYSQL> getConnection();
    bool releaseConnection(shared_ptr<MYSQL> connection);
    int getFreeConn() { return FreeConn_; }
    void destroySqlConnectionPool();
    static SqlConnectionPool& getInstance();
    void init(InetAddress localAddr, string username, string password, string dataBaseName, int MaxConn);
    void initSqlResult();
    unordered_map<string, string>& getUsersInfor() { return users_; }
    mutex& getMutex() { return mtx_; }
private:
    SqlConnectionPool();
    ~SqlConnectionPool();
    int MaxConn_;
    int CurConn_;   //现在有多少已使用连接
    int FreeConn_;  //还有多少剩余连接
    std::list<shared_ptr<MYSQL>> connList_;    //RAII机制
    InetAddress localAddr_;
    string username_;
    string password_;
    string databaseName_;
    Sem sem_;
    std::mutex mutex_;
    bool isInited_;
    std::unordered_map<string, string> users_;
    std::mutex mtx_;
};