#ifndef ERROR_HPP
#define ERROR_HPP

#include <exception>
#include <string>

enum PARSING_ERROR {
  BAD_REQUEST = 400,
  NOT_ACCEPTABLE = 406,
  REQUEST_TIMEOUT = 408,
  LENGTH_REQUIRED = 411,
  PAYLOAD_TOO_LARGE = 413,
  URL_TOO_LONG = 414,
  METHOD_NOT_IMPLEMENTED = 501,
  LONG_HEADER = 431,
  HTTP_VERSION_NOT_SUPPORTED = 505,
  INTERNAL_SERVER_ERROR = 500
};

class ParsingError : public std::exception {
private:
  PARSING_ERROR type;
  std::string metadata;

public:
  // Constructor accepts a const char* that is used to set
  // the exception message
  ParsingError(PARSING_ERROR type, std::string metadata);
  virtual ~ParsingError() throw();
  virtual const char *what() const throw() { return metadata.c_str(); }

  PARSING_ERROR get_type();
  std::string get_metadata();
};

#endif
