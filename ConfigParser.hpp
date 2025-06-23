/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.42.fr>          +#+  +:+       +#+        */
/*                                               =+   +#           */
/*   Created: 2025/06/14 18:33:49 by muel-bak          #+#    #+#             */
/*   Updated: 2025/06/21 16:09:12 by muel-bak         ###   ########.fr       */
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

#define MAX_BODY_SIZE 1048576

struct LocationConfig {
  std::string path;                         // e.g., "/api", "= /"
  std::vector<std::string> allowed_methods; // e.g., {"POST", "GET"}
  std::vector<HTTP_METHOD> allowed_methods2; // e.g., {"POST", "GET"}
  std::string root;                         // e.g., "/var/www/api"
  std::string alias;                        // e.g., "/var/www/static"
  std::vector<std::string> try_files; // e.g., {"$uri", "$uri/", "/index.html"}
  std::string proxy_pass;             // e.g., "http://backend:8080"
  std::map<std::string, std::string>
      proxy_set_headers;                        // e.g., {"Host", "$host"}
  std::string expires;                          // e.g., "1y"
  std::string access_log;                       // e.g., "off"
  std::string auth_basic;                       // e.g., "Restricted Area"
  std::string auth_basic_user_file;             // e.g., "/etc/nginx/.htpasswd"
  std::string deny;                             // e.g., "all"
  std::string cgi_ext;                          // e.g., ".php"
  std::string cgi_bin;                          // e.g., "/usr/bin/php-cgi"
  std::vector<LocationConfig> nested_locations; // For nested location blocks
  std::vector<std::string> index; // e.g., {"index.html", "index.htm"}
  int redirect_code;              // e.g., 301
  std::string redirect_url;       // e.g., "/newpath"
  // TODO: if not inherite it 
  bool autoindex;                 // e.g., true for "on"
  std::string upload_store;       // e.g., "/tmp/uploads"

  LocationConfig()
      : path(""), allowed_methods(), root(""), alias(""), try_files(),
        proxy_pass(""), proxy_set_headers(), expires(""), access_log(""),
        auth_basic(""), auth_basic_user_file(""), deny(""), cgi_ext(""),
        cgi_bin(""), nested_locations(), index(), redirect_code(0),
        redirect_url(""), autoindex(false), upload_store("") {}
};

class ServerConfig {
private:
  int fd;
  std::string host; // e.g., "0.0.0.0"
  int port;         // TODO: fail
  std::vector<std::string>
      server_names;            // e.g., {"example.com", "www.example.com"}
  size_t client_max_body_size; // e.g., 1048576 (1MB)
  // TODO: check if status code of the error is valid
  std::map<int, std::string> error_pages; // e.g., {404, "/404.html"}
  std::string root;                       // TODO: fail
  std::vector<std::string> index;         // e.g., {"index.html", "index.htm"}
  std::vector<LocationConfig> locations;
  bool autoindex; // e.g., true for "on"

public:
  // Constructor
  ServerConfig() : client_max_body_size(MAX_BODY_SIZE), autoindex(false) {
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

  // Setters
  void setFd(const int fd) { this->fd = fd; }
  void setHost(const std::string &h) { host = h; }
  void setPort(int p) {
    if (p < 0 || p > 65535) {
      throw std::runtime_error("Invalid port number");
    }
    port = p;
  }
  void setServerNames(const std::vector<std::string> &names) {
    server_names = names;
  }
  void setClientMaxBodySize(size_t size) { client_max_body_size = size; }
  void setErrorPages(const std::map<int, std::string> &pages) {
    error_pages = pages;
  }
  void setRoot(const std::string &r) { root = r; }
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

#endif // CONFIG_PARSER_HPP
