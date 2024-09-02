/*
 * @Author       : 晚乔最美
 * @Date         : 2023-06-16
 * @copyleft Apache 2.0
 */ 
#ifndef SQLCONNPOOL_H
#define SQLCONNPOOL_H

#include <mysql/mysql.h>
#include <string>
#include <queue>
#include <mutex>
#include <semaphore.h>
#include <thread>
#include "../log/log.h"

class SqlConnPool {
public:
    static SqlConnPool *Instance();

    // 获取一个连接
    MYSQL *GetConn();
    void FreeConn(MYSQL * conn);
    int GetFreeConnCount();

    void Init(const char* host, int port,
              const char* user,const char* pwd, 
              const char* dbName, int connSize);
    void ClosePool();

private:
    SqlConnPool();
    ~SqlConnPool();

    // 最大的连接数
    int MAX_CONN_;
    // 当前的用户数
    int useCount_;
    // 空闲的用户数
    int freeCount_;

    // 队列（MySQL *）
    std::queue<MYSQL *> connQue_;
    // 互斥锁
    std::mutex mtx_;
    // 信号量
    sem_t semId_;
};


#endif // SQLCONNPOOL_H