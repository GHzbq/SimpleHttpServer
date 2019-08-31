#include <errno.h>
#include "./Lock.h"
#include <glog/logging.h>

Sem::Sem() {
    int ret = ::sem_init(&m_sem, 0, 0);
    if (0 != ret) {
        LOG(ERROR) << "::sem_init() error, errno is " << errno << std::endl;
        return;
    }
}

Sem::~Sem() {
    int ret = ::sem_destroy(&m_sem);
    if (0 != ret) {
        LOG(ERROR) << "::sem_destory() error, errno is " << errno << std::endl;
        return;
    }
}

bool Sem::wait() {
    int ret = ::sem_wait(&m_sem);

    if (ret < 0) {
        LOG(ERROR) << "::sem_wait() error, errno is " << errno << std::endl;
        return false;
    }

    return true;
}

bool Sem::post() {
    int ret = ::sem_post(&m_sem);

    if (0 != ret) {
        LOG(ERROR) << "::sem_post() error, errno is " << errno << std::endl;
        return false;
    }

    return true;
}


Locker::Locker() {
    int ret = ::pthread_mutex_init(&m_mutex, nullptr);

    if (0 != ret) {
        LOG(ERROR) << "::pthread_mutex_init() error, errno is " << errno << std::endl;
        return;
    }
}

Locker::~Locker() {
    int ret = ::pthread_mutex_destroy(&m_mutex);

    if (0 != ret) {
        LOG(ERROR) << "::pthread_mutex_destroy() error, errno is " << errno << std::endl;
        return;
    }
}

bool Locker::lock() {
    int ret = ::pthread_mutex_lock(&m_mutex);

    if (0 != ret) {
        LOG(ERROR) << "::pthread_mutex_lock() error, errno is " << errno << std::endl;
        return false;
    }

    return true;
}

bool Locker::unlock() {
    int ret = ::pthread_mutex_unlock(&m_mutex);

    if (0 != ret) {
        LOG(ERROR) << "::pthread_mutex_unlock() error, errno is " << errno << std::endl;
        return false;
    }
    return true;
}


Cond::Cond() {
    int ret = pthread_mutex_init(&m_mutex, nullptr);
    if (0 != ret) {
        LOG(ERROR) << "::pthread_mutex_init() error, errno is " << errno << std::endl;
        return;
    }

    ret = ::pthread_cond_init(&m_cond, nullptr);
    if (0 != ret) {
        LOG(ERROR) << "::pthread_cond_init() error, errno is " << errno << std::endl;
        ::pthread_mutex_destroy(&m_mutex);
        return;
    }
}

Cond::~Cond() {
    int ret = ::pthread_mutex_destroy(&m_mutex);
    if (0 != ret) {
        LOG(ERROR) << "::pthread_mutex_destroy() error, errno is " << errno << std::endl;
    }
    ret = ::pthread_cond_destroy(&m_cond);
    if (0 != ret) {
        LOG(ERROR) << "::pthread_cond_destroy() error, errno is " << errno << std::endl;
    }
}

bool Cond::wait() {
    ::pthread_mutex_lock(&m_mutex);
    int ret = ::pthread_cond_wait(&m_cond, &m_mutex);
    ::pthread_mutex_unlock(&m_mutex);

    if (0 != ret) {
        LOG(ERROR) << "::pthread_cond_wait() error, errno is " << errno << std::endl;
        return false;
    }
    return true;
}

bool Cond::signal() {
    int ret = ::pthread_cond_signal(&m_cond);

    if (0 != ret) {
        LOG(ERROR) << "::pthread_cond_signal() error, errno is " << errno << std::endl;
        return false;
    }

    return true;
}
