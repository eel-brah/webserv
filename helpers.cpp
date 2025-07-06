
#include "helpers.hpp"
#include "parser.hpp"
#include <algorithm>

// std::vector<std::string> split(const std::string &str, char delimiter) {
//     std::vector<std::string> tokens;
//     size_t start = 0;
//     size_t end = str.find(delimiter);
//     
//     while (end != std::string::npos) {
//         tokens.push_back(str.substr(start, end - start)); // Extract substring
//         start = end + 1;
//         end = str.find(delimiter, start);
//     }
//     tokens.push_back(str.substr(start)); // Add the last token
//     return tokens;
// }

std::string join(const std::vector<std::string>& vec, const std::string& delimiter) {
    std::ostringstream oss;
    for (size_t i = 0; i < vec.size(); ++i) {
        oss << vec[i];
        if (i != vec.size() - 1) {
            oss << delimiter;  // Add delimiter between elements
        }
    }
    return oss.str();
}

// convert HTTP_METHOD to printable string
// returns "NONE" for NONE
std::string httpmethod_to_string(HTTP_METHOD method) {
  switch (method) {
    case GET:
      return "GET";
    case POST:
      return "POST";
    case OPTIONS:
      return "OPTIONS";
    case DELETE:
      return "DELETE";
    default:
      return "NONE";
  }
}

std::string httpversion_to_string(HTTP_VERSION method) {
  switch (method) {
    case HTTP1:
      return "HTTP1";
    case HTTP2:
      return "HTTP2";
  }
  return "not suppose to be reached";
}

std::string toLower(std::string str) {
  for (size_t i = 0; i < str.length(); i++) {
    str[i] = std::tolower(str[i]);
  }
  return str;
}



// trim from end of string (right)
std::string& rtrim(std::string& s, const char* t)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
std::string& ltrim(std::string& s, const char* t)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
std::string& trim(std::string& s, const char* t)
{
    return ltrim(rtrim(s, t), t);
}

void catch_setup_serverconf(Client *client, std::vector<ServerConfig> &servers_conf) {
  try {
    if (!client->get_request()->server_conf)
      client->get_request()->setup_serverconf(servers_conf, client->port);
  } catch (std::exception &e) {
    client->get_request()->server_conf = &servers_conf[0]; // set 0 as default
  }
}
