#pragma once 

#include <iostream>
#include <list>
#include <cstring>
#include <string>
#include <mutex>
#include <errno.h>
#include <mysql/mysql.h>
#include "../locker/Lock.h"

class MysqlConnectionPool {
public:
    MYSQL*  getConnection();                 // 获取数据库连接
    bool    releaseConnection(MYSQL* pConn); // 释放连接
    void    destroyPool();                   // 销毁所有连接

    // 单例模式获取一个连接
    // 这里参数都是const类型 参数不设置为引用的原因是可能传递的是一个 C风格字符串
    static MysqlConnectionPool* getInstance(const std::string  host,
                                            const std::string  user,
                                            const std::string  password,
                                            const std::string  databasename,
                                            const unsigned int port,
                                            const unsigned int maxconnection);
    unsigned int getMaxConnection() const ;
    unsigned int getCurrentConnection() const ;
    unsigned int getFreeConnection() const ;

    ~MysqlConnectionPool();

private:
    MysqlConnectionPool(const std::string& host,
                        const std::string& user,
                        const std::string& password,
                        const std::string& databasename,
                        const unsigned int port,
                        const unsigned int maxconnection); // 构造函数

    // 由于是单例模式 所以要禁掉拷贝构造和赋值运算符重载
    MysqlConnectionPool(const MysqlConnectionPool&) = delete;
    MysqlConnectionPool& operator=(const MysqlConnectionPool&) = delete;

private:
    unsigned int                 m_maxconnection;     // 最大连接数
    unsigned int                 m_currentconnection; // 当前已使用的连接数
    unsigned int                 m_freeconnection;    // 当前空闲的连接数

    static std::mutex            s_mutex;
    std::list<MYSQL*>            m_connectionlist;
    MysqlConnectionPool*         m_pconnectionpool;
    MYSQL*                       m_pconnection;

    static MysqlConnectionPool * volatile s_pinstance;

    std::string                  m_host;
    std::string                  m_user;
    std::string                  m_password;
    std::string                  m_databasename;
    unsigned int                 m_port;
};
