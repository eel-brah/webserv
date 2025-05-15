

#include "parser.hpp"
#include "helpers.hpp"

HttpRequest::HttpRequest() : method(NONE), body("/tmp/prefix_XXXXXX"), bodytmp(false) , head_parsed(false), body_len(0){

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
  while (!this->head_parsed) {
    if (raw_data.find('\n') == std::string::npos || !raw_data.compare(0, 2, "\r\n")) {
      //std::cout << "failed: " << raw_data << std::endl;
      if (!raw_data.compare(0, 2, "\r\n")) {
        raw_data = raw_data.substr(2, raw_data.size() - 2); // remove the trailing \r\n
        this->head_parsed = true;
      }
      break;
    }
    std::cout << "raw_data = " << raw_data << std::endl;
    line = raw_data.substr(0, raw_data.find('\n') + 1);

    // TODO: handle errors
    if (this->method == NONE) {
      this->parse_first_line(line);
    }
    else {
      this->parse_header(line);
    }
    raw_data = raw_data.substr(raw_data.find('\n') + 1, raw_data.size() - line.size()); // TODO: could segfault if \n is before \0
  }
  if (this->head_parsed) {
    read_body_loop(raw_data);
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

// TODO: maybe just throw if invalid_request
int HttpRequest::parse_first_line(std::string line) {
  std::vector<std::string> parts = split(line, ' ');
  if (parts.size() != 3) {
    // TODO: set response to invalid_request
  }
  if (this->set_method(parts[0])) {
    // TODO: set response to invalid_method
  }
  this->path = URL(parts[1]);
  if (set_httpversion(parts[2])) {
    // TODO: set response as invalid_http_version
  }
  return 0;
}

int HttpRequest::parse_header(std::string line) {
  //std::cout << "parse_header: " << line << std::endl;
  std::vector<std::string> parts = split(line, ':');
  if (parts.size() < 2) {
    // TODO: set response
    return 1;
  }
  HttpHeader header = HttpHeader();
  header.key = toLower(parts[0]);
  header.value = join(std::vector<std::string>(parts.begin() + 1, parts.end()), ":");
  this->headers.push_back(header);
  return 0;
}

void HttpRequest::print() {
  std::cout << "==============================\n";
  std::cout << "method: " << httpmethod_to_string(this->get_method()) << std::endl;
  std::cout << "path: ";
  this->get_path().debug_print();

  std::cout << "version: " << httpversion_to_string(this->get_version()) << std::endl;
  std::cout << "number of headers: " << this->headers.size() << std::endl;
  for (std::vector<HttpHeader>::iterator it = this->headers.begin(); it != this->headers.end(); it++) {
    std::cout << (*it).key << ": " << (*it).value << std::endl;
  }

  std::cout << "content-length parsed = " << this->get_content_len() << std::endl;
  std::cout << "body: " << this->body << std::endl;
  std::cout << "body_len: " << this->body_len << std::endl;
  std::cout << "==============================\n";
}

HTTP_METHOD HttpRequest::get_method() {
  return this->method;
}

HTTP_VERSION HttpRequest::get_version() {
  return this->http_version;
}

URL HttpRequest::get_path() {
  return this->path;
}

FILE *HttpRequest::get_body_fd(std::string perm) {
  int fd;
  if (!this->bodytmp) {
    fd = mkstemp((char *) this->body.c_str());
    this->bodytmp = true;
    return fdopen(fd, perm.c_str());
  } else {
    return fopen(this->body.c_str(), perm.c_str());
  }
}

HttpHeader HttpRequest::get_header_by_key(std::string key) {
  for (size_t i = 0; i < this->headers.size(); i++) {
    if (this->headers[i].key == key) {
      return this->headers[i];
    }
  }
  throw std::runtime_error("HttpRequest::get_header_by_key: key not found");
}

// return -1 when failed
size_t HttpRequest::get_content_len() {
  try {
    HttpHeader header = get_header_by_key("content-length");
    return std::atoi(header.value.c_str());
  } catch (std::exception &e) {
    return -1;
  }
}

// returns weather to stop
// TODO: use c++ iostream
bool HttpRequest::read_body_loop(std::string raw_data) {
  assert(this->head_parsed);
  FILE *body = this->get_body_fd("a");
  if (this->use_content_len() && this->body_len < this->get_content_len()) {
    if (this->body_len + raw_data.length() < this->get_content_len())
      std::fputs(raw_data.c_str(), body);
    else
      std::fputs(raw_data.substr(0, this->get_content_len() - this->body_len).c_str(), body);
    this->body_len += raw_data.length();
    fclose(body);
    return false;
  }
  return true;
}

bool HttpRequest::use_content_len() {
  return true; // TODO: true for now
}
