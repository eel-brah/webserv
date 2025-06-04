#include "webserv.hpp"

#include <cstdio>
#include <ctime>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>

#define ROOT current_path() + "/root"
#define ERRORS ROOT + "/errors"
#define DEFAULT "index.html"

#define HTTP_VERSION "HTTP/1.1"
#define SPACE " "
#define CRLF "\r\n"

#define FAIL_500                                                               \
  "<html> \
<head> \
	<title>500 Internal Server Error</title> \
</head> \
<body> \
	<center> \
		<h1>500 Internal Server Error</h1> \
	</center> \
	<hr> \
	<center>nginy / 0.0.1 </center> \
</body> \
</html> \
"
const size_t CHUNK_THRESHOLD = 1024 * 1024; // 1MB
const int chunk_size = 8192;

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
std::string get_server_header() {
  // TODO: Update server name
  return "Server: " + std::string("nginy/0.0.1 (Linux)") + CRLF;
}
bool ends_with(const std::string &str, const std::string &suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }
  return str.substr(str.length() - suffix.length()) == suffix;
}

#include <algorithm>
#include <map>
#include <string>

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

void print_response(std::string response) {
  std::cout << "+++++++++++++" << std::endl;
  std::cout << response << std::endl;
  std::cout << "+++++++++++++" << std::endl;
}
long get_file_size(const std::string &filepath) {
  struct stat file_stat;
  if (stat(filepath.c_str(), &file_stat) != 0) {
    std::cerr << "Error: Unable to stat file " << filepath << std::endl;
    return -1;
  }
  return file_stat.st_size;
}

std::string int_to_hex(int value) {
  std::stringstream ss;
  ss << std::hex << std::uppercase << value;
  return ss.str();
}
void send_in_chunks(int socket, const std::string &data,
                    size_t chunk_size = 4096) {
  size_t total_sent = 0;
  size_t data_size = data.size();

  while (total_sent < data_size) {
    size_t bytes_left = data_size - total_sent;
    size_t bytes_to_send = std::min(chunk_size, bytes_left);

    ssize_t sent_now =
        send(socket, data.c_str() + total_sent, bytes_to_send, 0);
    if (sent_now < 0) {
      throw std::runtime_error("send failed: " + std::string(strerror(errno)));
    }

    total_sent += sent_now;
  }
}

void send_file(Client &client, int socket, int fd, const std::string &file,
               int status_code) {
  std::string status_line;
  std::string headers = get_server_header() + get_date_header();
  std::string body;
  std::string response;
  ssize_t sent;

  struct stat st;
  if (fstat(fd, &st) == -1) {
    throw std::runtime_error("fstat failed: " + std::string(strerror(errno)));
  }
  if (!S_ISREG(st.st_mode)) {
    throw std::runtime_error("fd does not refer to a regular file");
  }
  // if (st.st_size < CHUNK_THRESHOLD) {
  if (1) {
    std::string content = read_file_to_str(fd, st.st_size);

    status_line = generate_status_line(status_code);
    if (status_code == 405) {
      headers += get_allow_header("GET");
    }
    headers += get_content_type(file);
    headers += get_content_length(content.size());
    headers += CRLF;
    body = content;

    response = status_line + headers + body;
    client.fill_response(response);
  } else {
    status_line = generate_status_line(status_code);
    headers += get_content_type(file);
    headers += get_transfer_encoding("chunked");
    headers += CRLF;

    response = status_line + headers;
    // print_response(response);
    sent = send(socket, response.c_str(), response.size(), 0);
    if (sent < 0) {
      throw std::runtime_error("send failed (headers): " +
                               std::string(strerror(errno)));
    }

    size_t size = st.st_size;
    char buffer[chunk_size];
    std::string chunk;
    while (true) {
      ssize_t bytes = read(fd, buffer, sizeof(buffer));
      if (bytes < 0) {
        throw std::runtime_error("read failed: " +
                                 std::string(strerror(errno)));
      }
      if (bytes == 0) {

        chunk = std::string("0") + CRLF + CRLF;
        sent = send(socket, chunk.c_str(), chunk.size(), 0);
        if (sent < 0) {
          throw std::runtime_error("send failed (final chunk): " +
                                   std::string(strerror(errno)));
        }
        return;
      }

      chunk = int_to_hex(bytes) + CRLF + std::string(buffer, bytes) + CRLF;
      sent = send(socket, chunk.c_str(), chunk.size(), 0);
      if (sent < 0) {
        throw std::runtime_error("send failed (chunk): " +
                                 std::string(strerror(errno)));
      }
    }
  }
}

void send_error(Client &client, int socket, int status_code) {
  std::string file = ERRORS + "/" + int_to_string(status_code) + ".html";
  int fd = open(file.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd == -1) {
    // TODO: For other failers
    std::string content = FAIL_500;
    std::string response = generate_status_line(status_code);
    response += get_server_header();
    response += get_date_header();
    response += get_content_type("500.html");
    response += get_content_length(content.size());
    response += CRLF;
    response += content;

    size_t sent = send(socket, response.c_str(), response.size(), 0);
    if (sent < 0) {
      throw std::runtime_error("send failed (headers): " +
                               std::string(strerror(errno)));
    }
  }
  send_file(client, socket, fd, file, status_code);
}
void handle_response(Client &client, int socket, HttpRequest *request) {
  // std::string method = request.get_method();request
  HTTP_METHOD method = request->get_method();

  std::string file = ROOT;
  std::string path = request->get_path().get_path();
  if (path == "/") {
    file += "/" + std::string(DEFAULT);
  } else {
    file += path;
  }

  if (method == HTTP_METHOD::GET) {
    int fd = open(file.c_str(), O_RDONLY | O_NONBLOCK);
    int error_code;
    if (fd == -1) {
      if (errno == ENOENT)
        error_code = 404;
      else if (errno == EACCES)
        error_code = 403;
      else
        error_code = 500;
      send_error(client, socket, error_code);
    } else {
      send_file(client, socket, fd, file, 200);
    }
  } else {
    send_error(client, socket, 405);
  }
}
