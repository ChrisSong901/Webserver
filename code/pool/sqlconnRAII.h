#ifndef SQLCONNRAII_H // 如果未定义 SQLCONNRAII_H，防止头文件被重复包含
#define SQLCONNRAII_H // 定义 SQLCONNRAII_H

#include "sqlconnpool.h" // 包含头文件 "sqlconnpool.h"，用于获取数据库连接池的定义

/* 资源在对象构造初始化，资源在对象析构时释放 */
class SqlConnRAII { // 定义一个名为 SqlConnRAII 的类
public: // 公有成员部分

    // 构造函数，接受一个指向指针的指针 sql 和一个指向 SqlConnPool 对象的指针 connpool 作为参数
    SqlConnRAII(MYSQL** sql, SqlConnPool *connpool) {
        assert(connpool); // 断言，确保连接池对象的有效性
        *sql = connpool->GetConn(); // 从连接池中获取一个数据库连接，并存储到指向指针的指针 sql 所指向的位置
        sql_ = *sql; // 将获取的数据库连接存储到成员变量 sql_ 中
        connpool_ = connpool; // 将连接池对象指针存储到成员变量 connpool_ 中
    }
    
    // 析构函数
    ~SqlConnRAII() {
        if(sql_) { connpool_->FreeConn(sql_); } // 如果数据库连接指针不为空，则调用连接池对象的 FreeConn 方法释放数据库连接
    }
    
private: // 私有成员部分

    MYSQL *sql_; // 指向 MYSQL 数据库连接的指针
    SqlConnPool* connpool_; // 指向数据库连接池对象的指针
};

#endif //SQLCONNRAII_H // 结束头文件保护宏的定义
