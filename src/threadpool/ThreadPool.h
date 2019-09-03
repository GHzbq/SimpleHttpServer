#pragma once

#include <iostream>
#include <list>

#include <pthread.h>
#include <glog/logging.h>

#include "../timer/Timer.h"
#include "../httpconnector/Connector.h"
#include "../blockqueue/BlockQueue.h"

template <typename  T>
class ThreadPool {
public:
    ThreadPool(int thread_number = 8, int max_requests = 10000) 
        : m_thread_number(thread_number)
        , m_max_requests(max_requests)
        , m_stop(false)
        , m_threads(nullptr)
        , m_timer_lst(nullptr)
        , m_timer(nullptr)
        , m_user_timer(nullptr)
    {
        if (thread_number <= 0 || max_requests <= 0) {
            LOG(ERROR) << "Invaild param, thread_number and max_requests all must be greater than 0, now thread_number is " << thread_number << ", and max_requests is " << max_requests << std::endl;
            return;
        }
        m_threads = new pthread_t[m_thread_number];
        if (!m_threads) {
            LOG(ERROR) << "allocate memory failed" << std::endl;
            return;
        }

        for (int i = 0; i < m_thread_number; ++i) {
            if (::pthread_create(m_threads+i, nullptr, worker, this) != 0) {
                delete[] m_threads;
                LOG(ERROR) << "::pthread_create failed, errno is " << errno  << std::endl;
                return;
            }
            if (::pthread_detach(m_threads[i]) != 0) {
                delete[] m_threads;
                LOG(ERROR) << "::pthread_detach failed, errno is " << errno << std::endl;
                return;
            }
        }
    }

    ~ThreadPool() {
        delete[] m_threads;
        m_stop = true;
    }

    bool append(T* request, int flagrw, SortTimerList* timer_lst, UtilTimer* timer, ClientData* user_timer) {
        if (m_work_queue.size() > m_max_requests) {
            LOG(INFO) << "request have already reached the upper limit, now work queue size is " << m_work_queue.size() << std::endl;
            return false;
        }
        m_work_queue.push(request);
        if (flagrw == 0) {
            request->m_flag = 0;
        } else if (flagrw == 1) {
            request->m_flag = 1;
        }
        m_timer_lst = timer_lst;
        m_timer = timer;
        m_user_timer = user_timer;
    }
    
private:
    static void* worker(void* arg) {
        ThreadPool *pool = static_cast<ThreadPool*>(arg);
        pool->run();
        return pool;
    }
    void run() {
        while (!m_stop) {
            T* request = nullptr;
            m_work_queue.pop(request);
            if (!request) {
                continue;
            }
            request->process(m_timer_lst, m_timer, m_user_timer);
        }
    }

private:
    int              m_thread_number;
    int              m_max_requests;
    pthread_t*       m_threads;
    BlockQueue<T*>   m_work_queue;
    Locker           m_queue_locker;
    bool m_stop;
    SortTimerList*   m_timer_lst;
    UtilTimer*       m_timer;
    ClientData*      m_user_timer;
};
