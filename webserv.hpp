#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "ConfigParser.hpp"
#include "parser.hpp"
#include <algorithm>
#include <arpa/inet.h>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <linux/limits.h>
#include <map>
#include <netdb.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

#define SPACE " "
#define CRLF "\r\n"

#define CONF_FILE "./nginy.conf"

#define MAX_EVENTS 100

// ustils
std::vector<std::string> split(const std::string &str, char del);
std::vector<std::string> split(const char *str, char del);
std::string current_path();
std::string read_file_to_str(const char *filename);
std::string read_file_to_str(const std::string &filename);
std::string read_file_to_str(int fd, size_t size);
void *get_in_addr(struct sockaddr *sa);
std::string int_to_string(int num);
std::string strip(const std::string &s);

template <typename T>
int find_in_vec(const std::vector<T> &vec, const T &target) {
  typename std::vector<T>::const_iterator it =
      std::find(vec.begin(), vec.end(), target);
  if (it != vec.end()) {
    return static_cast<int>(it - vec.begin());
  }
  return -1;
}

std::string int_to_hex(int value);

void sigchld_handler(int s);

void print_addrinfo(struct addrinfo *info);

// server
int start_server(std::vector<ServerConfig> &servers_conf);

// response
void process_request(Client &client);
void send_special_response(Client &client, int status_code, std::string info = "");
std::string special_response(int status_code);
bool handle_write(Client &client);

void generate_error(Client &client, int status_code);

// response utils
std::map<std::string, std::string> make_mime_map();
const std::string &get_status_code_phrase(int code);
std::string get_file_path(const std::string &root,
                          const std::vector<std::string> &index,
                          const std::string &path);
std::string get_date_header();
std::string get_server_header();
std::string get_content_type(std::string file);
std::string get_content_length(int size);
std::string get_transfer_encoding(const std::string &encoding);
std::string generate_status_line(int status_code);
std::string get_allow_header(std::string allowed_methods);
std::string get_location_header(std::string location);
std::string int_to_hex(int value);

// logs
enum LogLevel { INFO, WARNING, ERROR, DEBUG };

void log_message(LogLevel level, const std::string &msg, const char *file = "",
                 int line = 0);

#define LOG(level, msg)                                                        \
  ((level == INFO) ? log_message(level, msg)                                   \
                   : log_message(level, msg, __FILE__, __LINE__))

#define LOG_STREAM(level, expression)                                          \
  do {                                                                         \
    std::ostringstream __logstream__;                                          \
    __logstream__ << expression;                                               \
    LOG(level, __logstream__.str());                                           \
  } while (0)

#endif
