#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#include "ConfigParser.hpp"
#include "parser.hpp"

#define SPACE " "
#define CRLF "\r\n"

#define CONF_FILE "./nginy.conf"
#define SERVER_SOFTWARE "nginy/0.0.1"

#define MAX_EVENTS 100
#define CLIENT_TIMEOUT 100
#define CGI_TIMEOUT 5


extern std::map<int, Client *> cgi_to_client;

int start_server(std::vector<ServerConfig> &servers_conf);
int executeCGI(int epoll_fd, const ServerConfig &server_conf, const std::string &script_path,
               const LocationConfig *location, Client *client);
LocationConfig *get_location(std::vector<LocationConfig> &locations,
                             const std::string &path);
int handle_cgi(int epoll_fd, Client *client);
void wait_for_child();

// response
void process_request(int epoll_fd, Client &client);
void send_special_response(Client &client, int status_code,
                           std::string info = "");
std::string special_response(int status_code);
bool handle_write(Client &client);

// response utils
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
std::string join_paths(const std::string &path1, const std::string &path2);
std::string random_string();

// utils
std::vector<std::string> split(const std::string &str, char del);
std::vector<std::string> split(const char *str, char del);
std::string get_ip(const struct sockaddr *sa);
std::string read_file_to_str(const char *filename);
std::string read_file_to_str(const std::string &filename);
std::string read_file_to_str(int fd, size_t size);
void *get_in_addr(struct sockaddr *sa);
std::string int_to_string(int num);
std::string strip(const std::string &s);
int set_nonblocking(int server_fd);
std::string long_to_string(long num);
bool is_dir(const std::string &path);
std::string join_vec(const std::vector<std::string> &vec);
std::string decode_url(const std::string &encoded);
void discard_socket_buffer(int client_fd);
template <typename T>
int find_in_vec(const std::vector<T> &vec, const T &target) {
  typename std::vector<T>::const_iterator it =
      std::find(vec.begin(), vec.end(), target);
  if (it != vec.end()) {
    return static_cast<int>(it - vec.begin());
  }
  return -1;
}
const char* method_to_string(HTTP_METHOD method);

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
