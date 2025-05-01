


#ifndef PARSER_HPP
#define PARSER_HPP


#include <unordered_map>
#include <vector>
#include <string>
#include <iostream>
#include <sys/epoll.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>


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
  public:
    std::string key;
    std::string value;
};

class HttpRequest {
  private:
    HTTP_METHOD method;
    std::vector<HttpHeader> headers;
    std::string path;
    HTTP_VERSION http_version;
    std::string body; // TODO: could be too large
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
};

class URL {
  private:
    std::string path;
    std::unordered_map<std::string, std::string> queries;
    std::string hash;
  public:
    URL(std::string url);
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
