


#ifndef PARSER_HPP
#define PARSER_HPP

#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <vector>
#include <string>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <cassert>

typedef enum {
  GET,
  POST,
  OPTIONS,
  DELETE,
  NONE
} HTTP_METHOD;

typedef enum {
  HTTP1,
  HTTP2
} HTTP_VERSION;

class HttpHeader {
  public: // TODO: make public
    std::string key;
    std::string value;
};

class URL {
  private:
    std::string path;
    std::map<std::string, std::string> queries;
    std::string hash;
  public:
    URL(std::string url);
    URL();

    std::map<std::string, std::string> parse_queries(std::string raw_queries);
    void debug_print() const;
};

class HttpRequest {
  private:
    HTTP_METHOD method;
    std::vector<HttpHeader> headers;
    URL path;
    HTTP_VERSION http_version;
    std::string body; // TODO: could be too large
    bool bodytmp;
    bool head_parsed;
    size_t body_len;
  public:
    HttpRequest();
    HttpRequest *clone();

    int parse_raw(std::string &raw_data);
    int parse_first_line(std::string line); // method + path
    int set_method(std::string method);
    int set_httpversion(std::string version);
    int parse_header(std::string line);
    void print();
    HTTP_METHOD get_method();
    HTTP_VERSION get_version();
    URL get_path();
    FILE *get_body_fd(std::string perm);
    size_t get_content_len();
    HttpHeader get_header_by_key(std::string key);


    bool read_body_loop(std::string &raw_data);
    bool use_content_len();
    bool use_transfer_encoding();
    bool handle_transfer_encoded_body(std::string raw_data);
};



class Client {
  private:
    int client_socket;
    std::string remaining_from_last_request;
    HttpRequest *request;
  public:
    int recv(void *buffer, size_t len);
    ~Client();
    Client();
    Client(int client_socket);
    Client & operator = (const Client &client);

    bool parse_loop();
};

#endif
