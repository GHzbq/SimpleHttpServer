#pragma once

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <sys/epoll.h>

#include <pthread.h>
#include <glog/logging.h>

#include "../locker/Lock.h"
#include "../timer/Timer.h"


class Connector {
    static const int FILENAME_LEN = 200;
    static const int READ_BUFFER_SIZE = 2048;
    static const int WRITE_BUFFER_SIZE = 1024;
    enum METHOD {
        GET = 0,
        POST,
        HEAD,
        PUT,
        DELETE,
        TRACE,
        OPTIONS,
        CONNECT,
        PATH
    };
    enum CHECK_STATE {
        CHECK_STATE_REQUESTLINE = 0,
        CHECK_STATE_HEADER,
        CHECK_STATE_CONTENT
    };
    enum HTTP_CODE {
        NO_REQUEST,
        GET_REQUEST,
        BAD_REQUEST,
        NO_RESOURCE,
        FORBIDDEN_REQUEST,
        FILE_REQUEST,
        INTERNAL_ERROR,
        CLOSED_CONNECTION
    };

    enum LINE_STATUS {
        LINE_OK = 0,
        LINE_BAD,
        LINE_OPEN
    };

    int m_flag; // 读写数据标志位
public:
    Connector() {}
    ~Connector() {}
public:
    void init(int sockfd, const struct sockadd_in& addr);
    void closeConnection(bool realclose = true);
    void process(SortTimerList* timerlst, UtilTimer* timer, ClientData* usertimer);
    bool readOnce();
    bool write();
    sockaddr_in* getAddress() const ;
    void initMysqlResult();
private:
    void init();
    HTTP_CODE processRead();
    bool processWrite(HTTP_CODE ret);
    HTTP_CODE processRequestLine(char* text);
    HTTP_CODE parseHeader(char* text);
    HTTP_CODE parseContent(char* text);
    HTTP_CODE doRequest();
    char* getLine();
    LINE_STATUS parseLine();
    void unmap();
    bool addResponse(const char* format, ...);
    bool addContent(const char* content);
    bool addStatusLine(int status, const char* title);
    bool addHeaders(int contentlength);
    bool addContentType();
    bool addContentLength(int contentlength);
    bool addLinger();
    bool addBlackLine();
public:
    static int m_epollfd;
    static int m_user_count;
private:
    int                m_sockfd;
    struct sockaddr_in m_address;
    char               m_read_buf[READ_BUFFER_SIZE];
    int                m_read_index;
    int                m_check_index;
    int                m_start_line;
    char               m_write_buf[WRITE_BUFFER_SIZE];
    int                m_write_index;
    CHECK_STATE        m_check_state;
    METHOD             m_method;
    char               m_real_file[FILENAME_LEN];
    char*              m_url;
    char*              m_version;
    char*              m_host;
    int                m_content_length;
    bool               m_linger;
    char*              m_file_address;
    struct stat        m_file_stat;
    struct iovec       m_iv[2];
    int                m_iv_count;
    int                m_cgi;    // 是否启用的 POST
    char*              m_string; // 存储请求头数据
};
