#include <glog/logging.h>
#include "./MysqlPool.h"

// 类外初始化静态成员
std::mutex MysqlConnectionPool::s_mutex;
MysqlConnectionPool* volatile MysqlConnectionPool::s_pinstance = nullptr;

MysqlConnectionPool::MysqlConnectionPool(const std::string& host,
                                         const std::string& user,
                                         const std::string& password, 
                                         const std::string& databasename,
                                         const unsigned int port,
                                         const unsigned int maxconnection)
    : m_maxconnection(maxconnection)
    , m_currentconnection(0)
    , m_host(host)
    , m_user(user)
    , m_password(password)
    , m_databasename(databasename)
    , m_port(port)
{
    // 在这里 不能用mutex进行保护，我们在单利模式中
    // 采取了双检测加锁的方式
    // 如果实例真的不存在，只有申请到锁的线程，
    // 再次检测实例不存在时，才会创建
    // 此时已经使用mutex加锁了，
    // 如果再次加锁，相当于对一个锁加锁两次
    // 会造成死锁
    // s_mutex.lock();
    for (unsigned int i = 0; i < maxconnection; ++i) {
        MYSQL* conn = nullptr;
        conn = ::mysql_init(conn);
        if (nullptr == conn) {
            LOG(ERROR) << "::mysql_init() error, errno is " << errno << ", mysql_error() = " << ::mysql_error(conn) << std::endl;
        }
        conn = ::mysql_real_connect(conn, host.c_str(), user.c_str(), password.c_str(),
                                  databasename.c_str(), port, nullptr, 0);
        if (nullptr == conn) {
            LOG(ERROR) << "::mysql_real_connect() error, errno is " << errno << ", mysql_error() = " << ::mysql_error(conn) << std::endl;
        }
        m_connectionlist.push_back(conn);
        ++m_freeconnection;
    }
    // s_mutex.unlock();
}

MysqlConnectionPool* MysqlConnectionPool::getInstance(const std::string  host,
                                                      const std::string  user,
                                                      const std::string  password,
                                                      const std::string  databasename,
                                                      const unsigned int port,
                                                      const unsigned int maxconnection) {
    // 双检测加锁
    if (nullptr == s_pinstance) {
        s_mutex.lock();
        if (nullptr == s_pinstance) {
            s_pinstance = new MysqlConnectionPool(host, user, password, databasename, port, maxconnection);
        }
        s_mutex.unlock();
    } 
    return s_pinstance;
}

MYSQL* MysqlConnectionPool::getConnection() {
    MYSQL* conn = nullptr;
    s_mutex.lock();
    if (m_connectionlist.size() > 0) {
        conn = m_connectionlist.front();
        m_connectionlist.pop_front();

        --m_freeconnection;
        ++m_currentconnection;
    }
    s_mutex.unlock();
    return conn;
}

bool MysqlConnectionPool::releaseConnection(MYSQL* conn) {
    if (nullptr == conn) {
        LOG(INFO) << "MysqlConnectionPool::releaseConnection(), Parameter of conn is null which type is `MYSQL*`" << std::endl;
        return false; 
    }
    // 释放连接并不是真正的释放，而是归还给连接池
    s_mutex.lock();
    m_connectionlist.push_back(conn);
    ++m_freeconnection;
    --m_currentconnection;
    s_mutex.unlock();
    return true;
}

void MysqlConnectionPool::destroyPool(){
    s_mutex.lock();
    for (auto it = m_connectionlist.begin(); it != m_connectionlist.end(); ++it) {
        mysql_close(*it);
        // 考虑迭代器失效的问题，不在这里进行删除，直接在最后调用 clear() 方法
    }
    m_currentconnection = 0;
    m_freeconnection = 0;
    m_connectionlist.clear();
    s_mutex.unlock();
}

unsigned int MysqlConnectionPool::getMaxConnection() const {
    return m_maxconnection;
}

unsigned int MysqlConnectionPool::getCurrentConnection() const {
    return m_currentconnection;
}

unsigned int MysqlConnectionPool::getFreeConnection() const {
    return m_freeconnection;
}

MysqlConnectionPool::~MysqlConnectionPool() {
    destroyPool();
}
