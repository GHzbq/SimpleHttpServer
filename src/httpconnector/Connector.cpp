#include <map>
#include <string>

#include <mysql/mysql.h>
#include <glog/logging.h>

#include "./Connector.h"
#include "../mysqlpool/MysqlPool.h"

const static std::string ok_200_title = "OK";
const static std::string error_400_title = "Bad Request";
const static std::string error_400_form  = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const static std::string error_403_title = "Forbidden";
const static std::string error_403_form  = "You don't have permisson to get file from this server.\n";
const static std::string error_404_title = "Not Found";
const static std::string error_404_form  = "The requested file was not found on this server.\n";
const static std::string error_500_title = "Internal Error";
const static std::string error_500_form  = "There was an unusual problem serving the request file.\n";

// 但浏览器出现连接重置时，可能是网站根目录出错
// 或http响应格式出错
// 或者访问的文件中内容完全为空
const static std::string doc_root = "/root/project/SimpleHttpServer/wwwroot";

// 创建数据库连接池
// TODO: 为什么这里要创建数据库连接池？
MysqlConnectionPool* g_mysqlconnectionpool = MysqlConnectionPool::getInstance("localhost", "root", "Root@123", "simplehttpserver", 3306, 5);

// 将表中的用户名和密码放入map
// TODO: 为什么要这么做？
// 这么做的原因是，一次性将所有的用户信息加载到内存，
// 虽然占用内存较多，但是验证用户身份信息将更加快捷
std::map<std::string, std::string> g_map_users;


void Connector::init(int sockfd, const struct sockaddr_in& addr){
    m_sockfd = sockfd;
    m_address = addr;
    addfd(m_epollfd, sockfd, true);
    ++m_user_count;
    init();
}

void Connector::closeConnection(bool realclose) {
    if (realclose && (m_sockfd != -1)) {
        removefd(m_epollfd, m_sockfd);
        m_sockfd = -1;
        --m_user_count;
    }
}

void Connector::process(SortTimerList* timerlst, UtilTimer* timer, ClientData* usertimer) {
    if (m_flag == 0) {
        if (!readOnce()) {
            closeConnection();
            timer->m_cb_func( &usertimer[m_sockfd] );
            if (timer) {
                timerlst->delTimer(timer);
            }
        } else {
            if (timer) {
                time_t cur = time(nullptr);
                timer->m_expire = cur + 3 * TIMESLOT;
                LOG(INFO) << "adjust timer once";
                timerlst->adjustTimer(timer);
            }
            Connector::HTTP_CODE read_ret = processRead();
            if (read_ret == Connector::NO_REQUEST) {
                modfd(m_epollfd, m_sockfd, EPOLLIN);
                return;
            }
            bool write_ret = processWrite(read_ret);
            if (!write_ret) {
                closeConnection();
            }
            modfd(m_epollfd, m_sockfd, EPOLLOUT);
        }
    } else if (m_flag == 1) {
        if (!write()) {
            closeConnection();
        }
    }
}

bool Connector::readOnce() {

}

bool Connector::write() {

}

sockaddr_in* Connector::getAddress() const {

}

void Connector::initMysqlResult() {
    // 先从连接池中取出一个连接
    MYSQL * mysql = g_mysqlconnectionpool->getConnection();

    // 在user表中检索username，password，保存在全局的map中
    if (mysql_query(mysql, "SELECT username, password from user")) {
        LOG(ERROR) << "mysql_query() failed, err: " << mysql_error(mysql) << std::endl;
        return ; // 查询失败，直接返回得了
    }

    // 从表中检索完整的结果集
    MYSQL_RES *result = mysql_store_result(mysql);

    MYSQL_ROW row ;
    std::string username, password;
    while ((row = mysql_fetch_row(result))) {
        username = row[0];
        password = row[1];
        g_map_users[username] = password;
    }
    mysql_free_result(result);
    g_mysqlconnectionpool->releaseConnection(mysql);
}

// 初始化新接收的参数
void Connector::init() {
    m_check_state = Connector::CHECK_STATE_REQUESTLINE;
    m_linger = false;
    m_method = Connector::GET;
    m_url =  nullptr;
    m_version = nullptr;
    m_content_length = 0;
    m_host = nullptr;
    m_start_line = 0;
    m_check_index = 0;
    m_write_index = 0;
    m_cgi = 0;
    memset(m_read_buf, 0x00, READ_BUFFER_SIZE);
    memset(m_write_buf, 0x00, WRITE_BUFFER_SIZE);
    memset(m_real_file, 0x00, FILENAME_LEN);
}

Connector::HTTP_CODE    Connector::processRead() {
    Connector::LINE_STATUS line_status = Connector::LINE_OK;
    Connector::HTTP_CODE   ret = Connector::NO_REQUEST;
    char* text = nullptr;
    while ((m_check_state == Connector::CHECK_STATE_CONTENT &&
            line_status == Connector::LINE_OK) ||
           ((line_status = parseLine()) == Connector::LINE_OK)) {
        text = getLine();
        m_start_line = m_check_index;
        LOG(INFO) << "get a new line: " << text << std::endl;
        switch(m_check_state) {
        case CHECK_STATE_REQUESTLINE:
            {
                ret = parseRequestLine(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                }
                break;
            }
        case CHECK_STATE_HEADER:
            {
                ret = parseHeader(text);
                if (ret == BAD_REQUEST) {
                    return BAD_REQUEST;
                } else if (ret == GET_REQUEST) {
                    return doRequest();
                }
                break;
            }
        case CHECK_STATE_CONTENT:
            {
                ret = parseContent(text);
                if (ret == GET_REQUEST) {
                    return doRequest();
                }
                line_status = LINE_OPEN;
                break;
            }
        default:
            return INTERNAL_ERROR;
        } // end of switch
    } // end of while
    return NO_REQUEST;
}

bool Connector::processWrite(HTTP_CODE ret) {
    switch (ret) {
    case INTERNAL_ERROR:
        {
            addStatusLine(500, error_500_title);
            addHeaders(error_500_form.size());
            if (!addContent(error_500_form)) {
                return false;
            }
            break;
        }
    case BAD_REQUEST:
        {
            addStatusLine(404, error_404_title);
            addHeaders(error_404_form.size());
            if (!addContent(error_404_form)) {
                return false;
            }
            break;
        }
    case FORBIDDEN_REQUEST:
        {
            addStatusLine(403, error_403_title);
            addHeaders(error_403_form.size());
            if (!addContent(error_403_form)) {
                return false;
            }
            break;
        }
    case FILE_REQUEST:
        {
            addStatusLine(200, ok_200_title);
            if (m_file_stat.st_size != 0) {
                addHeaders(m_file_stat.st_size);
                m_iv[0].iov_base = m_write_buf;
                m_iv[0].iov_len = m_write_index;
                m_iv[1].iov_base = m_file_address;
                m_iv[1].iov_len = m_file_stat.st_size;
                m_iv_count = 2;
                return true;
            } else {
                std::string ok_string = "<html><body></body></html>";
                addHeaders(ok_string.size());
                if (!addContent(ok_string)) {
                    return false;
                }
            }
            break;
        }
    default:
        return false;
    }
    m_iv[0].iov_base = m_write_buf;
    m_iv[0].iov_len = m_write_index;
    m_iv_count = 1;
    return true;
}

Connector::HTTP_CODE Connector::parseRequestLine(char* text) {
    m_url = strpbrk(text, " \t"); // 标记 " \t" 中任意一个字符在text中出现的位置 
    if (!m_url) {
        return BAD_REQUEST;
    }
    *m_url = '\0';
    ++m_url;
    char* method = text;
    if (strcasecmp(method, "GET") == 0) {
        // GET请求
        m_method = GET;
    } else if (strcasecmp(method, "POST") == 0) {
        // POST请求
        m_method = POST;
        m_cgi = 1;
    } else {
        return BAD_REQUEST;
    }

    m_url += strspn(m_url, " \t");
    m_version = strpbrk(m_url, " \t");
    if (!m_version) {
        return BAD_REQUEST;
    }
    *m_version = '\0';
    ++m_version;
    m_version += strspn(m_version, " \t");
    if (strcasecmp(m_url, "HTTP/1.1") != 0) {
        return BAD_REQUEST;
    }
    if (strncasecmp(m_url, "http://", 7) == 0) {
        m_url += 7;
        m_url = strchr(m_url, '/');
    }
    if (!m_url || m_url[0] != '/') {
        return BAD_REQUEST;
    }
    if (strlen(m_url) == 1) {
        strcat(m_url, "judge.html");
    }
    m_check_state = CHECK_STATE_HEADER;
    return NO_REQUEST;
}

Connector::HTTP_CODE Connector::parseHeader(char* text) {
    if (text[0] == '\0') {
        if (m_content_length != 0) {
            m_check_state = CHECK_STATE_CONTENT;
            return NO_REQUEST;
        }
        return GET_REQUEST;
    } else if (strncasecmp(text, "Connection:", 11) == 0) {
        text += 11;
        text += strspn(text, " \t");
        if (strcasecmp(text, "keep-alive") == 0) {
            m_linger = true;
        }
    } else if (strncasecmp(text, "Content-length:", 15) == 0) {
        text += 15;
        text += strspn(text, " \t");
        m_content_length = atol(text);
    } else if (strncasecmp(text, "Host:", 5) == 0) {
        text += 5;
        text += strspn(text, " \t");
        m_host = text;
    } else {
        LOG(INFO) << "unknown header: " << text << std::endl;
    }

    return NO_REQUEST;
}

Connector::HTTP_CODE Connector::parseContent(char* text) {

}

Connector::HTTP_CODE Connector::doRequest() {

}

char* Connector::getLine() {

}

Connector::LINE_STATUS  Connector::parseLine() {

}

void Connector::unmap() {

}

bool Connector::addResponse(const std::string& format, ...) {

}

bool Connector::addContent(const std::string& content) {

}

bool Connector::addStatusLine(int status, const std::string& title) {

}

bool Connector::addHeaders(int contentlength) {

}

bool Connector::addContentType() {

}

bool Connector::addContentLength(int contentlength) {

}

bool Connector::addLinger() {

}

bool Connector::addBlackLine() {

}
