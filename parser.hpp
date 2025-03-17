


#ifndef PARSER_HPP
#define PARSER_HPP


#include <vector>

enum HTTP_METHOD {
  GET,
  POST,
  OPTIONS,
  DELETE
};

// TODO
class HttpHeader {

};

// TODO
class HttpRequest {
  private:
    HTTP_METHOD method;
    std::vector<HttpHeader> headers;
};

#endif
