
#include "helpers.hpp"
#include "parser.hpp"

std::vector<std::string> split(const std::string &str, char delimiter) {
    std::vector<std::string> tokens;
    size_t start = 0;
    size_t end = str.find(delimiter);
    
    while (end != std::string::npos) {
        tokens.push_back(str.substr(start, end - start)); // Extract substring
        start = end + 1;
        end = str.find(delimiter, start);
    }
    tokens.push_back(str.substr(start)); // Add the last token
    return tokens;
}

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
