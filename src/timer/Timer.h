/**
 * 升序链表
 * tick 相当于一个心搏函数
 * 每隔一段固定时间就执行一次
 * 检测并处理到期的任务
 *
 * 判断定时任务到期的依据是定时器的 expire 值 小于当前系统时间
 *
 * 从执行效率上看，添加定时器的时间复杂度是 O(n)
 *                 删除定时器的时间复杂度是 O(1)
 *                 执行定时任务的时间复杂度是 O(1)
 * */
#pragma once

#include <time.h>
#include <netinet/in.h>

static const int TIMESLOT  = 5;  // 最小超时单位
static const int BUFF_SIZE = 64; 

class UtilTimer;

struct ClientData {
    struct sockaddr_in s_address;
    int                s_sockfd;
    char               s_buf[ BUFF_SIZE ];
    UtilTimer*         s_timer;
};

class UtilTimer {
public:
    UtilTimer()
        : m_prev(nullptr)
        , m_next(nullptr)
    {}
public:
    time_t      m_expire;
    void      (*m_cb_func)(ClientData*);
    ClientData* m_user_data;
    UtilTimer*  m_prev;
    UtilTimer*  m_next;
};

class SortTimerList {
public:
    SortTimerList();
    ~SortTimerList();
    void addTimer(UtilTimer* timer);
    void adjustTimer(UtilTimer* timer);
    void delTimer(UtilTimer* timer);
    void tick();
private:
    void addTimer(UtilTimer* timer, UtilTimer* lstHead);
private:
    UtilTimer* m_head;
    UtilTimer* m_tail;
};
