

#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <string>
#include <vector>
#include <sstream>
#include "parser.hpp"
#include "webserv.hpp"

# define CONSUME_BEGINNING(x, len) (x).substr((len), (x).size() - (len))

#define WS " \t\n\r\f\v"

// std::vector<std::string> split(const std::string &str, char delimiter);
std::string join(const std::vector<std::string>& vec, const std::string& delimiter);

std::string httpmethod_to_string(HTTP_METHOD method);
std::string httpversion_to_string(HTTP_VERSION method);

std::string toLower(std::string str);

std::string& rtrim(std::string& s, const char* t = WS);
std::string& ltrim(std::string& s, const char* t = WS);
std::string& trim(std::string& s, const char* t = WS);

void catch_setup_serverconf(Client *client, std::vector<ServerConfig> &servers_conf);

std::string decode_url(const std::string& encoded);
std::string replace_first(const std::string& str, const std::string& old_sub, const std::string& new_sub);
std::string clean_path(const std::string url);

std::string random_string();
std::string bufferToHexString(const uint8_t* buffer, size_t length);
#endif
