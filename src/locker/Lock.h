#pragma once

#include <pthread.h>
#include <semaphore.h>

class Sem {
public:
    Sem();

    ~Sem();

    bool wait();

    bool post();
private:
    sem_t m_sem;
};


class Locker {
public:
    Locker();

    ~Locker();

    bool lock();

    bool unlock();
private:
    pthread_mutex_t m_mutex;
};


class Cond {
public:
    Cond();

    ~Cond();

    bool wait();

    bool signal();
private:
    pthread_mutex_t m_mutex;
    pthread_cond_t  m_cond;
};
