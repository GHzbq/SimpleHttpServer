#include "Timer.h"
#include <glog/logging.h>

SortTimerList::SortTimerList()
    : m_head(nullptr)
    , m_tail(nullptr)
{}

SortTimerList::~SortTimerList() {
    UtilTimer* tmp = m_head;
    while (tmp) {
        m_head = tmp->m_next;
        delete tmp;
        tmp = m_head;
    }
}

void SortTimerList::addTimer(UtilTimer* timer) {
    if (!timer) {
        return;
    }

    if (!m_head) {
        m_head = m_tail = timer;
        return;
    }

    if (timer->m_expire < m_head->m_expire) {
        timer->m_next = m_head;
        m_head->m_prev = timer;
        m_head = timer;
        return;
    }

    addTimer(timer, m_head);
}

void SortTimerList::adjustTimer(UtilTimer* timer) {
    if (!timer) {
        return;
    }
    UtilTimer* tmp = timer->m_next;
    if (!tmp || (timer->m_expire < tmp->m_expire)) {
        return;
    }

    if (timer == m_head) {
        m_head = m_head->m_next;
        m_head->m_prev = nullptr;
        timer->m_next = nullptr;
        addTimer(timer, m_head);
    }
    else {
        timer->m_prev->m_next = timer->m_next;
        timer->m_next->m_prev = timer->m_prev;
        addTimer(timer, timer->m_next);
    }
}

void SortTimerList::delTimer(UtilTimer* timer) {
    if (!timer) {
        return;
    }

    if ((timer == m_head) && (timer == m_tail)) {
        delete timer;
        m_head = nullptr;
        m_tail = nullptr;
        return;
    }

    if (timer == m_head) {
        m_head = m_head->m_next;
        m_head->m_prev = nullptr;
        delete timer;
        return;
    }

    if (timer == m_tail) {
        m_tail = m_tail->m_prev;
        m_tail->m_next = nullptr;
        delete timer;
        return;
    }

    timer->m_prev->m_next = timer->m_next;
    timer->m_next->m_prev = timer->m_prev;
    delete timer;
}

void SortTimerList::tick() {
    if (!m_head) {
        return;
    }
    LOG(INFO) << "timer tick" << std::endl;
    time_t cur = time(nullptr);
    UtilTimer* tmp = m_head;
    while (tmp) {
        if (cur < tmp->m_expire) {
            break;
        }

        tmp->m_cb_func(tmp->m_user_data);
        m_head = tmp->m_next;
        if (m_head) {
            m_head->m_prev = nullptr;
        }
        delete tmp;
        tmp = m_head;
    }
}

void SortTimerList::addTimer(UtilTimer* timer, UtilTimer* lstHead) {
    UtilTimer* prev = lstHead;
    UtilTimer* tmp  = prev->m_next;
    while (tmp) {
        if (timer->m_expire < tmp->m_expire) {
            prev->m_next = timer;
            timer->m_next = tmp;
            tmp->m_prev = timer;
            break;
        }
        prev = tmp;
        tmp = tmp->m_next;
    }

    /**
     * 遍历完 lstHead 节点之后的部分链表
     * 仍未找到超时时间大于目标定时器超时时间的节点
     * 则将目标定时器插入链表尾部
     * 并把它设置为链表新的尾节点
     * */
    if (!tmp) {
        prev->m_next = timer;
        timer->m_prev = prev;
        timer->m_next = nullptr;
    }
}
