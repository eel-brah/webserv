

#include "../include/parser.hpp"
#include "../include/helpers.hpp"
#include "../include/errors.hpp"


std::string URL::normalize_url(std::string url) {
    std::string decoded = decode_url(url);
    decoded = clean_path(decoded);
    return decoded;
}

// parse the url
// raises error when fails
URL::URL(std::string url) {
  if (url[0] != '/') {
    throw ParsingError(BAD_REQUEST, ""); // TODO: idk if this the correct response
  }

  std::string::size_type has_query = url.find('?');
  std::string::size_type has_hash = url.find('#');
  std::string query_part = "";
  std::string hash_part = "";
  std::string pathname = url;

  // # has priority
  if (has_query != std::string::npos && has_hash != std::string::npos && has_hash < has_query) {
    has_query = std::string::npos;
  }
  if (has_query != std::string::npos)
    query_part = url.substr(has_query, has_hash != std::string::npos ? has_hash - has_query : url.length() - has_query);
  if (has_hash != std::string::npos)
    hash_part = url.substr(has_hash, url.length() - has_hash);

  if (has_query != std::string::npos)
    pathname = url.substr(0, has_query);
  else if (has_hash != std::string::npos)
    pathname = url.substr(0, has_hash);


  this->path = this->normalize_url(pathname);
  this->coded_path = pathname;
  this->hash = decode_url(hash_part);
  this->queries = decode_url(query_part);
  this->coded_queries = query_part;
  if (this->queries.size() > 0)
      this->queries.erase(0, 1);
  //this->queries = this->parse_queries(query_part);
}

std::map<std::string, std::string> URL::parse_queries(std::string raw_queries) {
  if (raw_queries.length() == 0)
    return std::map<std::string, std::string>(); // empty
                                                 //
  assert(raw_queries[0] == '?'); // if this hit, it's probably a bug

  std::map<std::string, std::string> result;
  std::vector<std::string> keyvalues = split(raw_queries, '&');
  //keyvalues[0] = keyvalues[0].substr(1, keyvalues[0].length());
  keyvalues[0] = CONSUME_BEGINNING(keyvalues[0], 1); // remove the trailing ?
  for (size_t i = 0; i < keyvalues.size(); i++) {
    std::vector<std::string> keyvalue = split(keyvalues[i], '=');
    if (keyvalue.size() == 2) { // igonore non-well-formatted querries // TODO: check the rfc if not lazy, but ig this is fine
      result.insert(std::pair<std::string, std::string>(keyvalue[0], keyvalue[1])); // TODO: check the rfc for behaviour when 2 keys with same name
    }
  } 
  return result;
}

void URL::debug_print() const {
  // Print the path
  std::cout << "Path: " << path << std::endl;

  // Print the queries in a nice way
  std::cout << "Queries: " << std::endl;
  /*
  for (std::map<std::string, std::string>::const_iterator it = queries.begin(); it != queries.end(); ++it) {
    std::cout << "  " << it->first << " = " << it->second << std::endl;
  }
  */

  // Print the hash
  std::cout << "Hash: " << hash << std::endl;
}

URL::URL() {
  this->path = "";
  this->queries = "";
  this->hash = "";
}
