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

#include "parser.hpp"

#define MAX_EVENTS 100
#define PORT "9999"

// ustils
std::vector<std::string> split(const std::string &str, char del);
std::vector<std::string> split(const char *str, char del);
std::string current_path();
std::string read_file_to_str(const char *filename);
std::string read_file_to_str(const std::string &filename);
std::string read_file_to_str(int fd, size_t size);
void *get_in_addr(struct sockaddr *sa);
std::string int_to_string(int num);

void sigchld_handler(int s);

void print_addrinfo(struct addrinfo *info);

// server
int start_server();

// response
void handle_response(Client &client, int connection, HttpRequest *req);
