


#include "errors.hpp"


ParsingError::ParsingError(PARSING_ERROR type, std::string metadata) : type(type), metadata(metadata) {

}


PARSING_ERROR ParsingError::get_type() {
  return this->type;
}

std::string ParsingError::get_metadata() {
  return this->metadata;
}
