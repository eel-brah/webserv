/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#             */
/*   Updated: 2025/07/26 12:59:34 by muel-bak         ###   ########.fr       */
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

typedef enum { GET, POST, OPTIONS, DELETE, NONE } HTTP_METHOD;

#define DEFAULT_MAX_BODY_SIZE (2 * 1024 * 1024) // 2MB default

struct LocationConfig {
  std::string path;                         // e.g., "/api", "= /"
  std::vector<std::string> allowed_methods; // e.g., {"POST", "GET"}
  std::vector<HTTP_METHOD> allowed_methods2; // e.g., {POST, GET}
  std::string root;                         // e.g., "/var/www/api"
  std::string alias;                        // e.g., "/var/www/static"
  std::vector<std::string> index;           // e.g., {"index.html", "index.htm"}
  int redirect_code;                        // e.g., 301
  std::string redirect_url;                 // e.g., "/newpath"
  bool autoindex;                           // e.g., true for "on"
  std::string upload_store;                 // e.g., "/tmp/uploads"
  std::map<std::string, std::string> cgi_ext; // e.g., {".php", "/usr/bin/php-cgi", ".py", "/usr/bin/python3", ".js", "/usr/bin/node"}

  LocationConfig()
      : path(""), allowed_methods(), root(""), alias(""),
        index(), redirect_code(0), redirect_url(""), autoindex(false),
        upload_store("") {}
};

class ServerConfig {
private:
  int fd;
  std::string host; // e.g., "0.0.0.0"
  int port;         // Mandatory
  std::vector<std::string>
      server_names;            // e.g., {"example.com", "www.example.com"}
  size_t client_max_body_size; // e.g., DEFAULT_MAX_BODY_SIZE (2MB)
  std::map<int, std::string> error_pages; // e.g., {404, "/404.html"}
  std::string root;                       // Mandatory
  std::vector<std::string> index;         // e.g., {"index.html", "index.htm"}
  std::vector<LocationConfig> locations;
  bool autoindex; // e.g., true for "on"
  bool has_listen; // Track if listen directive was set
  bool has_root;   // Track if root directive was set

public:
  ServerConfig() : client_max_body_size(DEFAULT_MAX_BODY_SIZE), autoindex(false), has_listen(false), has_root(false) {
    fd = -1;
  }
  ~ServerConfig() { close(this->fd); }

  // Getters
  int getFd() const { return fd; }
  std::string getHost() const { return host; }
  int getPort() const { return port; }
  const std::vector<std::string> &getServerNames() const {
    return server_names;
  }
  size_t getClientMaxBodySize() const { return client_max_body_size; }
  const std::map<int, std::string> &getErrorPages() const {
    return error_pages;
  }
  std::string getRoot() const { return root; }
  const std::vector<std::string> &getIndex() const { return index; }
  const std::vector<LocationConfig> &getLocations() const { return locations; }
  bool isAutoindex() const { return autoindex; }
  bool hasListen() const { return has_listen; }
  bool hasRoot() const { return has_root; }

  // Setters
  void setFd(const int fd) { this->fd = fd; }
  void setHost(const std::string &h) { host = h; }
  void setPort(int p) {
    if (p < 0 || p > 65535) {
      throw std::runtime_error("Invalid port number");
    }
    port = p;
    has_listen = true;
  }
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

  // Methods to add individual elements
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

std::vector<ServerConfig> parseConfig(const std::string &file);
bool isPathCompatible(const std::string &locationPath,
                      const std::string &requestedPath);
bool endsWith(const std::string &str, const std::string &suffix);

#endif // CONFIG_PARSER_HPP
