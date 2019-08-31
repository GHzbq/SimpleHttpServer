#include <map>

#include "./Connector.h"
#include <glog/logging.h>
#include <mysql/mysql.h>

const static char* ok_200_title = "OK";
const static char* error_400_title = "Bad Request";
const static char* error_400_form  = "Your request has bad syntax or is inherently impossible to staisfy.\n";
const static char* error_403_title = "Forbidden";
const static char* error_403_form  = "You don't have permisson to get file from this server.\n";
const static char* error_404_title = "Not Found";
const static char* error_404_form  = "The requested file was not found on this server.\n";
const static char* error_500_title = "Internal Error";
const static char* error_500_form  = "There was an unusual problem serving the request file.\n";

const static char* doc_root = "";
