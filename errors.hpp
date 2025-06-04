


#ifndef ERROR_HPP
#define ERROR_HPP


#include <exception>
#include <string>

enum PARSING_ERROR {
  BAD_REQUEST,
  METHOD_NOT_ALLOWED,
  METHOD_NOT_IMPLEMENTED,
  LONG_HEADER,
  HTTP_VERSION_NOT_SUPPORTED,
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
