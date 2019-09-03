#include <iostream>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <string>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <signal.h>

#include <glog/logging.h>

#include "./src/locker/Lock.h"
#include "./src/timer/Timer.h"
#include "./src/httpconnector/Connector.h"
#include "./src/mysqlpool/MysqlPool.h"
#include "./src/threadpool/ThreadPool.h"

const static int MAX_FD = 65535;
const static int MAX_EVENT_NUMBER = 10000 ;

extern int addfd(int epollfd, int fd, bool one_shot);
extern int remove(int epollfd, int fd);
extern int setnonblocking(int fd);

static int g_pipefd[2];
static SortTimerList g_timer_lst;
static int g_epollfd;

// 信号处理函数
void sig_handler(int sig) {
    int save_errno = errno;
    int msg = sig;
    ::send(g_pipefd[1], (char*)&msg, 1, 0);
    errno = save_errno;
}

// 设置信号函数
void addsig(int sig, void(handler)(int), bool restart = true) {
    (void)sig;
    struct sigaction sa;
    memset(&sa, 0x00, sizeof(sa));
    sa.sa_handler = handler;
    if (restart) {
        sa.sa_flags |= SA_RESTART;
    }
    sigfillset(&sa.sa_mask);
    if (::sigaction(sig, &sa, nullptr) != 0) {
        LOG(ERROR) << "::sigaction() failed, errno is "  << errno << std::endl;
        return;
    }
}

// 定时处理任务 通过alarm函数发送SIGALRM信号
void timer_handler() {
    g_timer_lst.tick();
    alarm( TIMESLOT );
}

// 
void cb_func(ClientData* user_data) {
    if (!user_data) {
        return;
    }
    epoll_ctl(g_epollfd, EPOLL_CTL_DEL, user_data->s_sockfd, 0);
    close(user_data->s_sockfd);
    LOG(INFO) << "close fd " << user_data->s_sockfd << std::endl;
}

void show_error(int connfd, const std::string info) {
    std::cout << info << std::endl;
    ::send(connfd, info.c_str(), info.size(), 0);
    close(connfd);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cout << "Usage: " << argv[0] << " port" << std::endl;
        return 1;
    }
    google::InitGoogleLogging("mylog.log");
    google::SetLogDestination(google::INFO, "./log/");

    int port = atoi(argv[1]);

    // 设置SIGPIPE信号的处理方式为忽略
    addsig(SIGPIPE, SIG_IGN);
    
    // 创建线程池
    ThreadPool<Connector>* threadpool = new ThreadPool<Connector>;

    // 创建数据库连接池
    MysqlConnectionPool* mysqlconnectionpool = MysqlConnectionPool::getInstance("localhost", "root", "Root@123", "simplehttpserver", 3306, 5);

    Connector* users = new Connector[MAX_FD];
    assert(users);

    users->initMysqlResult();

    // 创建套接字
    int listenfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    assert(listenfd >= 0);
    struct linger tmp = {1, 0};

    // 若有数据待发送，延迟关闭
    ::setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &tmp, sizeof(tmp));
    int ret = 0;
    struct sockaddr_in serverAddr;
    bzero(&serverAddr, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(port);

    int opt = 1;
    ::setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    ret = ::bind(listenfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));
    assert(ret >= 0);

    // 创建内核事件表
    epoll_event events[MAX_EVENT_NUMBER];
    int epollfd = ::epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd, false);
    Connector::m_epollfd = epollfd;

    // 创建管道
    ret = ::socketpair(PF_UNIX, SOCK_STREAM, 0, g_pipefd);
    assert( ret != -1 );
    setnonblocking(g_pipefd[1]);
    addfd(epollfd, g_pipefd[0], false);

    addsig(SIGALRM, sig_handler, false);
    addsig(SIGTERM, sig_handler, false);
    bool stop_server = false;

    ClientData* users_timer = new ClientData[MAX_FD];
    bool timeout = false;
    alarm(TIMESLOT);

    while (!stop_server) {
        int number = ::epoll_wait(epollfd, events, MAX_EVENT_NUMBER, -1);
        if (number < 0 && errno != EINTR) {
            LOG(INFO) << "::epoll_wait() failed, errno is " << errno << std::endl;
            break;
        }

        for (int i = 0; i < number; ++i) {
            int sockfd = events[i].data.fd;

            // 处理新到的客户连接
            if (sockfd == listenfd) {
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLen = sizeof(clientAddr);
                int connfd = ::accept(listenfd, (struct sockaddr*)&clientAddr, &clientAddrLen);
                if (connfd < 0) {
                    LOG(ERROR) << "::accept() failed, errno is " << errno << std::endl;
                    continue;
                }
                if (Connector::m_user_count >= MAX_FD) {
                    show_error(connfd, "Internal server busy");
                    LOG(ERROR) << "Internal server busy" << std::endl;
                    continue;
                }
                users[connfd].init(connfd, clientAddr);

                users_timer[connfd].s_address = clientAddr;
                users_timer[connfd].s_sockfd = connfd;
                UtilTimer* timer = new UtilTimer;
                timer->m_user_data = &users_timer[connfd];
                timer->m_cb_func = cb_func;
                time_t cur = time(nullptr);
                timer->m_expire = cur + 3 * TIMESLOT;
                users_timer[connfd].s_timer = timer;
                g_timer_lst.addTimer(timer);
            } else if (events[i].events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) {
                users[sockfd].closeConnection();
                cb_func(&users_timer[sockfd]);
                UtilTimer* timer = users_timer[sockfd].s_timer;
                if (timer) {
                    g_timer_lst.delTimer(timer);
                }
            } else if ( (sockfd == g_pipefd[0]) && (events[i].events & EPOLLIN) ) {
                char signals[1024];
                ret = ::recv(g_pipefd[0], signals, sizeof(signals), 0);
                if (ret == -1) {
                    continue;
                } else if (ret == 0) {
                    continue;
                } else {
                    for (int i = 0; i < ret; ++i) {
                        switch (signals[i]) {
                        case SIGALRM:
                            {
                                timeout = true;
                                break;
                            }
                        case SIGTERM:
                            {
                                stop_server = true;
                            }
                        }
                    }
                }
            } else if (events[i].events & EPOLLIN) {
                threadpool->append(users+sockfd, 0, &g_timer_lst, users_timer[sockfd].s_timer, &users_timer[sockfd]);
            } else if (events[i].events & EPOLLOUT) {
                threadpool->append(users+sockfd, 1, &g_timer_lst, users_timer[sockfd].s_timer, &users_timer[sockfd]);
            }
        }

        if (timeout) {
            timer_handler();
            timeout = false;
        }
    }
    close(epollfd);
    close(listenfd);
    close(g_pipefd[1]);
    close(g_pipefd[0]);
    delete[] users;
    delete[] users_timer;
    delete threadpool;
    mysqlconnectionpool->destroyPool();

    return 0;
}
