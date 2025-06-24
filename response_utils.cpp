#include "webserv.hpp"

#define HTTP_VERSION "HTTP/1.1"

#define DEFAULT "index.html"

const std::string &get_status_code_phrase(int code) {
  static std::map<int, std::string> codes;
  static std::string unknown = "Unknown";

  if (codes.empty()) {
    // 1xx (Informational): The request was received, continuing process
    codes[100] = "Continue";
    codes[101] = "Switching Protocols";
    codes[102] = "Processing";
    codes[103] = "Early Hints";

    // 2xx (Successful): The request was successfully received, understood, and
    // accepted
    codes[200] = "OK";
    codes[201] = "Created";
    codes[202] = "Accepted";
    codes[203] = "Non-Authoritative Information";
    codes[204] = "No Content";
    codes[205] = "Reset Content";
    codes[206] = "Partial Content";
    codes[207] = "Multi-Status";
    codes[208] = "Already Reported";
    codes[226] = "IM Used";

    // 3xx (Redirection): Further action needs to be taken in order to complete
    // the request
    codes[300] = "Multiple Choices";
    codes[301] = "Moved Permanently";
    codes[302] = "Found";
    codes[303] = "See Other";
    codes[304] = "Not Modified";
    codes[305] = "Use Proxy";
    codes[307] = "Temporary Redirect";
    codes[308] = "Permanent Redirect";

    // 4xx (Client Error): The request contains bad syntax or cannot be
    // fulfilled
    codes[400] = "Bad Request";
    codes[401] = "Unauthorized";
    codes[403] = "Forbidden";
    codes[404] = "Not Found";
    codes[405] = "Method Not Allowed";
    codes[406] = "Not Acceptable";
    codes[407] = "Proxy Authentication Required";
    codes[408] = "Request Timeout";
    codes[409] = "Conflict";
    codes[410] = "Gone";
    codes[411] = "Length Required";
    codes[412] = "Precondition Failed";
    codes[413] = "Payload Too Large";
    codes[414] = "URI Too Long";
    codes[415] = "Unsupported Media Type";
    codes[416] = "Range Not Satisfiable";
    codes[417] = "Expectation Failed";
    codes[418] = "I'm a teapot";
    codes[421] = "Misdirected Request";
    codes[422] = "Unprocessable Entity";
    codes[423] = "Locked";
    codes[424] = "Failed Dependency";
    codes[425] = "Too Early";
    codes[426] = "Upgrade Required";
    codes[428] = "Precondition Required";
    codes[429] = "Too Many Requests";
    codes[431] = "Request Header Fields Too Large";
    codes[451] = "Unavailable For Legal Reasons";

    // 5xx (Server Error): The server failed to fulfill an apparently valid
    // request
    codes[500] = "Internal Server Error";
    codes[501] = "Not Implemented";
    codes[502] = "Bad Gateway";
    codes[503] = "Service Unavailable";
    codes[504] = "Gateway Timeout";
    codes[505] = "HTTP Version Not Supported";
    codes[506] = "Variant Also Negotiates";
    codes[507] = "Insufficient Storage";
    codes[508] = "Loop Detected";
    codes[510] = "Not Extended";
    codes[511] = "Network Authentication Required";
  }

  std::map<int, std::string>::iterator it = codes.find(code);
  if (it != codes.end()) {
    return it->second;
  }

  return unknown;
}

std::map<std::string, std::string> make_mime_map() {
  std::map<std::string, std::string> m;
  m["html"] = "text/html";
  m["htm"] = "text/html";
  m["txt"] = "text/plain";
  m["mp4"] = "video/mp4";
  m["jpg"] = "image/jpeg";
  m["jpeg"] = "image/jpeg";
  m["png"] = "image/png";
  m["css"] = "text/css";
  m["js"] = "application/javascript";
  return m;
}

std::string get_file_path(const std::string &root,
                          const std::vector<std::string> &index,
                          const std::string &path) {
  if (path == "/") {
    if (index.empty()) {
      return "";
    }
    std::string file_path = root + "/";
    std::string::size_type base_len = file_path.length();
    struct stat buf;
    for (std::vector<std::string>::const_iterator it = index.begin();
         it != index.end(); ++it) {
      file_path.resize(base_len);
      file_path += *it;
      if (stat(file_path.c_str(), &buf) == 0 && S_ISREG(buf.st_mode)) {
        return file_path;
      }
    }
    return "";
  } else {
    return root + path;
  }
}

std::string generate_status_line(int status_code) {
  std::string status_code_phrase = get_status_code_phrase(status_code);
  if (status_code_phrase == "Unknown") {
    LOG(WARNING, "Unknown status code");
  }
  return std::string(HTTP_VERSION) + SPACE + int_to_string(status_code) +
         SPACE + status_code_phrase + CRLF;
}

// Required
std::string get_date_header() {
  time_t now = time(0);

  tm *utc_time = gmtime(&now);

  if (!utc_time) {
    throw std::runtime_error("gmtime failed");
  }
  char buffer[50];
  strftime(buffer, sizeof(buffer), "%a, %d %b %Y %H:%M:%S GMT", utc_time);

  return "Date: " + std::string(buffer) + CRLF;
}

// Optional
std::string get_server_header() {
  // TODO: Update server name
  return "Server: " + std::string("nginy/0.0.1 (Linux)") + CRLF;
}

// for 201 and 3xx
// std::string get_location_header() {
//   // TODO:generate location header for resource creation (201) or redirection
//   // (3xx)
//   return "Location: ";
// }

// Optional, used with 503 or 3xx
// std::string get_retry_after_header() {
//   // TODO:generate Retry-After for 503 Service Unavailable or 3xx Redirection
//   return "Retry-After: ";
// }

// for 405
std::string get_allow_header(std::string allowed_methods) {
  // TODO: for 405 Method Not Allowed
  return "Allow: " + allowed_methods + CRLF;
}

std::string get_location_header(std::string location) {
  // TODO: for 405 Method Not Allowed
  return "Location: " + location + CRLF;
}
// Highly advisable
// std::string get_vary_header() {
//   //TODO:
//   return "Vary: ";
// }

std::string get_mime_type(const std::string &filepath) {
  static const std::map<std::string, std::string> mime_types = make_mime_map();

  size_t dot_pos = filepath.find_last_of('.');
  if (dot_pos != std::string::npos) {
    std::string ext = filepath.substr(dot_pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    std::map<std::string, std::string>::const_iterator it =
        mime_types.find(ext);
    if (it != mime_types.end()) {
      return it->second;
    }
  }
  return "application/octet-stream";
}

std::string get_content_type(std::string file) {
  return "Content-Type: " + get_mime_type(file) + CRLF;
}

std::string get_content_length(int size) {
  return "Content-Length: " + int_to_string(size) + CRLF;
}

std::string get_transfer_encoding(const std::string &encoding) {
  return "Transfer-Encoding: " + encoding + CRLF;
}

std::string int_to_hex(int value) {
  std::stringstream ss;
  ss << std::hex << std::uppercase << value;
  return ss.str();
}
