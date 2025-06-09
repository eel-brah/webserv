


#ifndef ERROR_HPP
#define ERROR_HPP


#include <exception>
#include <string>

enum PARSING_ERROR {
  BAD_REQUEST = 400,
  NOT_ACCEPTABLE = 406,//The requested content type is not acceptable based on the Accept header.
  REQUEST_TIMEOUT = 408,//The server timed out waiting for the client to send the full request.
  LENGTH_REQUIRED = 411,
  PAYLOAD_TOO_LARGE = 413,
  URL_TOO_LONG = 414,// A server that receives a request-target longer than any URI it wishes to parse MUST respond with a 414 (URI Too Long)
  METHOD_NOT_ALLOWED = 405,
  METHOD_NOT_IMPLEMENTED = 501,
  LONG_HEADER = 431,
  HTTP_VERSION_NOT_SUPPORTED = 505
};


class ParsingError : public std::exception {
private:
  PARSING_ERROR type;
  std::string metadata;

public:
    // Constructor accepts a const char* that is used to set
    // the exception message
    ParsingError(PARSING_ERROR type, std::string metadata);


    PARSING_ERROR get_type();
    std::string get_metadata();
};

#endif
