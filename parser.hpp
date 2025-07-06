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
#include <cstring>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include <linux/limits.h>
#include "ConfigParser.hpp"


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

    std::string get_path(){
      return this->path;
    }
    std::map<std::string, std::string> parse_queries(std::string raw_queries);
    void debug_print() const;
};

class HttpRequest {
  public:
    std::string body; // TODO: could be too large
  private:
    HTTP_METHOD method;
    std::vector<HttpHeader> headers;
    URL path;
    HTTP_VERSION http_version;
    bool bodytmp;
    bool body_parsed;
    size_t body_len;
    std::fstream body_tmpfile;
  public:
    bool head_parsed;
    ServerConfig *server_conf;
    HttpRequest();
    ~HttpRequest();
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
    ssize_t get_content_len();
    HttpHeader *get_header_by_key(std::string key);


    bool read_body_loop(std::string &raw_data);
    bool use_content_len();
    bool use_transfer_encoding();
    bool handle_transfer_encoded_body(std::string &raw_data);
    size_t push_to_body(std::string &raw_data, size_t max);

    bool request_is_ready();

    std::fstream& get_body_tmpfile();

    void setup_serverconf(std::vector<ServerConfig> &servers_conf, std::string port);
};



class Client {
  private:
    int client_socket;
    std::string remaining_from_last_request;
    HttpRequest *request;

    Client();
  public:
    std::string port;
    //ServerConfig *server_conf;
    std::string response;
    size_t write_offset;
    bool chunk;
    int response_fd;
    std::string current_chunk;
    size_t chunk_offset;
    bool final_chunk_sent;
    bool connected;

    int recv(void *buffer, size_t len);
    ~Client();
    Client(int client_socket);
    Client & operator = (const Client &client);

    int get_socket(){
      return this->client_socket;
    }
    HttpRequest* get_request(){
      return this->request;
    }
    bool parse_loop();
    std::string get_response(){
      return this->response;
    }

    void fill_response(std::string response){
      this->response = response;
    }

    void clear_request(){
      delete this->request;
      this->request = NULL;
      remaining_from_last_request.clear();
    }

};

#endif
