#pragma once

#include <iostream>
#include <cstdlib>

#include <sys/time.h>

#include <pthread.h>
#include <glog/logging.h>

template <class T>
class BlockQueue {
public:
    BlockQueue(int maxsize = 10000) 
        : m_mutex(nullptr)
        , m_cond (nullptr)
        , m_array(nullptr)
        , m_size (0)
        , m_maxsize(maxsize)
        , m_front(0)
        , m_back (0)
    {
        if (maxsize <= 0) {
            m_maxsize = 0;
            LOG(ERROR) << "Invaild param maxsize, maxsize must be greater than 0, now maxsize is " << maxsize << std::endl;
            return;
        }

        m_array = new T[m_maxsize];
        m_mutex = new pthread_mutex_t;
        m_cond  = new pthread_cond_t;
        pthread_mutex_init(m_mutex, nullptr);
        pthread_cond_init(m_cond,   nullptr);
    }

    ~BlockQueue() {
        pthread_mutex_lock(m_mutex);
        if (m_array != nullptr) {
            delete m_array;
            m_array = nullptr;
        }
        pthread_mutex_unlock(m_mutex);
        pthread_cond_destroy(m_cond);
        pthread_mutex_destroy(m_mutex);

        delete m_cond;
        delete m_mutex;
    }

    void clear() {
        pthread_mutex_lock(m_mutex);
        m_size = 0;
        m_front = 0;
        m_back = 0;
        pthread_mutex_unlock(m_mutex);
    }

    bool full()    const {
        pthread_mutex_lock(m_mutex);
        if (m_maxsize == m_size) {
            pthread_mutex_unlock(m_mutex);
            return true;
        }
        pthread_mutex_unlock(m_mutex);
        return false;
    }

    bool empty()   const {
        pthread_mutex_lock(m_mutex);
        if (0 == m_size) {
            pthread_mutex_unlock(m_mutex);
            return true;
        }
        pthread_mutex_unlock(m_mutex);
        return false;
    }

    int  size()    const {
        int tmp = 0;
        pthread_mutex_lock(m_mutex);
        tmp = m_size;
        pthread_mutex_unlock(m_mutex);
        return tmp;
    }

    int  maxSize() const {
        int tmp = 0;
        pthread_mutex_lock(m_mutex);
        tmp = m_maxsize;
        pthread_mutex_unlock(m_mutex);
        return tmp;
    }

    bool push(const T& item) {
        pthread_mutex_lock(m_mutex);
        if (m_size >= m_maxsize) {
            pthread_cond_broadcast(m_cond);
            pthread_mutex_unlock(m_mutex);
            return false;
        }
        m_array[m_back] = item;
        ++m_size;
        ++m_back;
        if (m_back >= m_maxsize) {
            m_back %= m_maxsize;
        }
        pthread_cond_broadcast(m_cond);
        pthread_mutex_unlock(m_mutex);

        return true;
    }
    bool pop (T& item) {
        pthread_mutex_lock(m_mutex);
        while (m_size <= 0) {
            if (0 != pthread_cond_wait(m_cond, m_mutex)) {
                pthread_mutex_unlock(m_mutex);
                return false;
            }
        }

        item = m_array[m_front];
        --m_size;
        ++m_front;
        if (m_front >= m_maxsize) {
            m_front %= m_maxsize;
        }
        pthread_mutex_unlock(m_mutex);
        return true;
    }
private:
    pthread_mutex_t*         m_mutex;
    pthread_cond_t *         m_cond;
    T*                       m_array;
    int                      m_size;
    int                      m_maxsize;
    int                      m_front;
    int                      m_back;
};
