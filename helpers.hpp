

#ifndef HELPERS_HPP
#define HELPERS_HPP

#include <string>
#include <vector>
#include <sstream>
#include "parser.hpp"

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

#endif
