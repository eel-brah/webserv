/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#             */
/*   Updated: 2025/08/10 12:34:06 by muel-bak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

const size_t MAX_STRING_LENGTH = 1024;
const size_t MAX_VECTOR_SIZE = 100;
const size_t MAX_MAP_SIZE = 50;
const long MAX_PORT = 65535;
const long MAX_ERROR_CODE = 599;
const long MIN_ERROR_CODE = 100;
const long MAX_REDIRECT_CODE = 399;
const long MIN_REDIRECT_CODE = 300;
const long MAX_BODY_SIZE = 1073741824L;

typedef enum { GET, POST, OPTIONS, DELETE, NONE } HTTP_METHOD;

#define DEFAULT_MAX_BODY_SIZE (2 * 1024 * 1024) // 2MB default

struct LocationConfig {
  std::string path;
  std::vector<std::string> allowed_methods;
  std::vector<HTTP_METHOD> allowed_methods2;
  std::string root;
  std::string alias;
  std::vector<std::string> index;
  int redirect_code;
  std::string redirect_url;
  bool autoindex;
  std::string upload_store;
  std::map<std::string, std::string> cgi_ext;

  LocationConfig()
      : path(""), allowed_methods(), root(""), alias(""), index(),
        redirect_code(0), redirect_url(""), autoindex(false), upload_store("") {
  }
};

class ServerConfig {
private:
  std::vector<int> fds;
  std::map<std::string, int> inter_ports;
  std::vector<std::string> server_names;
  size_t client_max_body_size;
  std::map<int, std::string> error_pages;
  std::string root;
  std::vector<std::string> index;
  std::vector<LocationConfig> locations;
  bool autoindex;
  bool has_listen;
  bool has_root;

public:
  ServerConfig()
      : client_max_body_size(DEFAULT_MAX_BODY_SIZE), autoindex(false),
        has_listen(false), has_root(false) {}
  ~ServerConfig() {
    for (int i = 0; i < (int)fds.size(); i++)
      close(this->fds[i]);
  }

  // Getters
  std::map<std::string, int> getInterPort() const { return inter_ports; }
  std::vector<int> getFds() const { return fds; }
  const std::vector<std::string> &getServerNames() const {
    return server_names;
  }
  size_t getClientMaxBodySize() const { return client_max_body_size; }
  const std::map<int, std::string> &getErrorPages() const {
    return error_pages;
  }
  std::string getRoot() const { return root; }
  const std::vector<std::string> &getIndex() const { return index; }
  std::vector<LocationConfig> &getLocations() { return locations; }
  bool isAutoindex() const { return autoindex; }
  bool hasListen() const { return has_listen; }
  bool hasRoot() const { return has_root; }

  // Setters
  void setInterPort(std::string inter, int port) {
    this->inter_ports[inter] = port;
  }
  void addFd(const int fd) { this->fds.push_back(fd); }
  void setServerNames(const std::vector<std::string> &names) {
    server_names = names;
  }
  void setClientMaxBodySize(size_t size) { client_max_body_size = size; }
  void setErrorPages(const std::map<int, std::string> &pages) {
    error_pages = pages;
  }
  void setRoot(const std::string &r) {
    root = r;
    has_root = true;
  }
  void setIndex(const std::vector<std::string> &idx) { index = idx; }
  void setLocations(const std::vector<LocationConfig> &locs) {
    locations = locs;
  }
  void setAutoindex(bool ai) { autoindex = ai; }

  void addServerName(const std::string &name) { server_names.push_back(name); }
  void addErrorPage(int code, const std::string &path) {
    if (code < 100 || code > 599) {
      std::stringstream ss;
      ss << code;
      throw std::runtime_error("Invalid error code: " + ss.str());
    }
    error_pages[code] = path;
  }
  void addIndex(const std::string &idx) { index.push_back(idx); }
  void addLocation(const LocationConfig &loc) { locations.push_back(loc); }
};

bool safeAtoi(const std::string &str, long &result);
std::vector<ServerConfig> parseConfig(const std::string &file);
bool isPathCompatible(const std::string &locationPath,
                      const std::string &requestedPath);
bool endsWith(const std::string &str, const std::string &suffix);

#endif