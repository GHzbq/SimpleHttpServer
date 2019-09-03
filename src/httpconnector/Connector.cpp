#include <map>
#include <string>

#include <mysql/mysql.h>
#include "./Connector.h"
#include <glog/logging.h>

const static std::string ok_200_title = "OK";
const static std::string error_400_title = "Bad Request";
const static std::string error_400_form  = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const static std::string error_403_title = "Forbidden";
const static std::string error_403_form  = "You don't have permisson to get file from this server.\n";
const static std::string error_404_title = "Not Found";
const static std::string error_404_form  = "The requested file was not found on this server.\n";
const static std::string error_500_title = "Internal Error";
const static std::string error_500_form  = "There was an unusual problem serving the request file.\n";

const static std::string doc_root = "/root/project/SimpleHttpServer/wwwroot";



void         Connector::init(int sockfd, const struct sockaddr_in& addr){

}
void         Connector::closeConnection(bool realclose = true) {

}
void         Connector::process(SortTimerList* timerlst, UtilTimer* timer, ClientData* usertimer) {

}
bool         Connector::readOnce() {

}

bool         Connector::write() {

}

sockaddr_in* Connector::getAddress() const {

}

void         Connector::initMysqlResult() {

}

void         Connector::init() {

}

Connector::HTTP_CODE    Connector::processRead() {

}

bool         Connector::processWrite(HTTP_CODE ret) {

}

Connector::HTTP_CODE    Connector::processRequestLine(char* text) {

}

Connector::HTTP_CODE    Connector::parseHeader(char* text) {

}
Connector::HTTP_CODE    Connector::parseContent(char* text) {

}

Connector::HTTP_CODE    Connector::doRequest() {

}

char*        Connector::getLine() {

}

Connector::LINE_STATUS  Connector::parseLine() {

}

void         Connector::unmap() {

}

bool         Connector::addResponse(const char* format, ...) {

}

bool         Connector::addContent(const char* content) {

}

bool         Connector::addStatusLine(int status, const char* title) {

}

bool         Connector::addHeaders(int contentlength) {

}

bool         Connector::addContentType() {

}

bool         Connector::addContentLength(int contentlength) {

}

bool         Connector::addLinger() {

}

bool         Connector::addBlackLine() {

}
