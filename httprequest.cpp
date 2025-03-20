

#include "parser.hpp"

HttpRequest::HttpRequest() : method(NONE) {

}

HttpRequest *HttpRequest::clone() {
  HttpRequest *result = new HttpRequest();
  result->method = this->method;
  result->headers = this->headers;
  result->path = this->path;
  return result;
}

int HttpRequest::parse_raw(std::string &raw_data) {

  std::string line;
  while (true) {
    if (raw_data.find('\n') == std::string::npos || !raw_data.compare(0, 4, "\r\n\r\n")) {
      //std::cout << "failed: " << raw_data << std::endl;
      break;
    }
    line = raw_data.substr(0, raw_data.find('\n') + 1);
    /*
    std::cout << raw_data << std::endl;
    std::cout << "raw_data.find('\\n') = " << raw_data.find('\n') << std::endl;
    std::cout << "line = " << line << std::endl;
    */
    // TODO: handle errors
    if (this->method == NONE) {
      this->parse_first_line(line);
    }
    else {
      this->parse_header(line);
    }
    raw_data = raw_data.substr(raw_data.find('\n'), raw_data.size() - line.size());
  }
  return 0;
}

int HttpRequest::set_method(std::string method) {
  if (method == "GET")
    this->method = GET;
  else if (method == "POST")
    this->method = POST;
  else if (method == "OPTIONS")
    this->method = OPTIONS;
  else if (method == "DELETE")
    this->method = DELETE;
  else
    return 1;
  return 0;
}

int HttpRequest::set_httpversion(std::string version) {
  if (version == "http/1.1")
    this->http_version = HTTP1;
  else if (version == "http/2")
    this->http_version = HTTP2;
  else
    return 1;
  return 0;
}

int HttpRequest::parse_first_line(std::string line) {
  std::vector<std::string> parts = split(line, ' ');
  if (parts.size() != 3) {
    // TODO: set response to invalid_request
  }
  if (this->set_method(parts[0])) {
    // TODO: set response to invalid_method
  }
  this->path = parts[1];
  if (set_httpversion(parts[2])) {
    // TODO: set response as invalid_http_version
  }
  return 0;
}

int HttpRequest::parse_header(std::string line) {
  //std::cout << "parse_header: " << line << std::endl;
  std::vector<std::string> parts = split(line, ':');
  if (parts.size() != 2) {
    // TODO: set response
    return 1;
  }
  HttpHeader header = HttpHeader();
  header.key = parts[0];
  header.value = parts[1];
  this->headers.push_back(header);
  return 0;
}

void HttpRequest::print() {
  std::cout << "method" << this->method << " " << GET << std::endl;
  for (std::vector<HttpHeader>::iterator it = this->headers.begin(); it != this->headers.end(); it++) {
    std::cout << (*it).key << ": " << (*it).value << std::endl;
  }
  std::cout << this->path << std::endl;
}
