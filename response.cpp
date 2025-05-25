#include "webserv.hpp"

#include <ctime>
#include <map>
#include <stdexcept>
#include <string>

#define ROOT current_path()
#define DEFAULT "index.html"

#define HTTP_VERSION "HTTP/1.1"
#define SPACE " "
#define CRLF "\r\n"

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
    codes[402] = "Payment Required";
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

std::string generate_status_line(int status_code) {
  // TODO: handle unknown status_code
  std::string status_code_phrase = get_status_code_phrase(status_code);
  if (status_code_phrase == "Unknown") {
    throw std::runtime_error("Unknown status code");
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

// Highly advisable
// std::string get_vary_header() {
//   //TODO:
//   return "Vary: ";
// }

// Optional
std::string get_server_header(std::string allowed_methods) {
  // TODO: Update server name
  return "Server: " + std::string("Test/0.0.1 (Linux)") + CRLF;
}
bool ends_with(const std::string &str, const std::string &suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }
  return str.substr(str.length() - suffix.length()) == suffix;
}
std::string get_content_type(std::string file) {
  (void)file;
  // if (ends_with(file, ".html"))
  //   return "Content-Type: " + std::string("text/html") + CRLF;
  // else
  //   return "Content-Type: " + std::string("text/plain") + CRLF;
  return "Content-Type: " + std::string("text/html") + CRLF;
}

std::string get_content_length(int size) {
  return "Content-Length: " + int_to_string(size) + CRLF;
}
void handle_response(int socket, HttpRequest *request) {
  // std::string method = request.get_method();request
  HTTP_METHOD method = request->get_method();
  std::string status_line;
  std::string headers = get_date_header();
  std::string body;
  std::string response;

  // int status_code = 200;
  // std::string status_line = generate_status_line(status_code);

  // TODO: Content-Length or Transfer-Encoding For responses with a message
  // body, No body is allowed in 1xx, 204, 304, or 2xx responses to CONNECT, so
  // these headers are prohibited WWW-Authenticate for 401.
  // std::string headers = get_date_header() + CRLF;

  // std::string response = status_line + "Content-type: text/html\r\n";

  // if (req->get_method() not in methods){
  //   [Server responds]
  //   HTTP/1.1 501 Not Implemented
  //   Content-Type: text/plain
  //   PURGE method not supported
  // }
  // 405 Method Not Allowed
  if (method == HTTP_METHOD::GET) {
    std::string file = ROOT;
    std::string path = request->get_path().get_path();
    if (path == "/") {
      file += "/" + std::string(DEFAULT);
    } else {
      file += path;
    }
    std::cout << file << "\n" << path << std::endl;
    int fd = access(file.c_str(), R_OK);
    if (fd == -1) {
      status_line = generate_status_line(404);
      headers += get_content_type("");
      headers += get_content_length(23);
      headers += CRLF;
      body = "<h1>Page Not Found</h1>";

      response = status_line + headers + body;
    } else {
      status_line = generate_status_line(200);
      std::string content = read_file_to_str(file.c_str());

      headers += get_content_type(file);
      headers += get_content_length(content.size());
      headers += CRLF;
      body = content;

      response = status_line + headers + body;
    }
  } else{
      status_line = generate_status_line(405);
      headers += get_allow_header("GET");
      headers += get_content_type("");
      headers += get_content_length(29);
      headers += CRLF;
      body = "<h1>Method not supported</h1>";

      response = status_line + headers + body;
  }
  std::cout << "+++++++++++++" << std::endl;
  std::cout << response << std::endl;
  std::cout << "+++++++++++++" << std::endl;

  send(socket, response.c_str(), response.size(), 0);
}
