/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#             */
/*   Updated: 2025/07/26 13:02:34 by muel-bak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ConfigParser.hpp"
#include "../include/helpers.hpp"

std::string trim(const std::string &str) {
  size_t start = 0;
  while (start < str.length() && (str[start] == ' ' || str[start] == '\t' ||
                                  str[start] == '\n' || str[start] == '\r')) {
    ++start;
  }
  if (start == str.length()) {
    return "";
  }
  size_t end = str.length() - 1;
  while (end > start && (str[end] == ' ' || str[end] == '\t' ||
                         str[end] == '\n' || str[end] == '\r')) {
    --end;
  }
  return str.substr(start, end - start + 1);
}

std::vector<std::string> split_with_quotes(const std::string &s) {
  std::vector<std::string> tokens;
  std::string token;
  bool in_quotes = false;
  for (size_t i = 0; i < s.length(); ++i) {
    if (s[i] == '"') {
      in_quotes = !in_quotes;
      token += s[i];
      continue;
    }
    if (!in_quotes && (s[i] == ' ' || s[i] == '\t')) {
      if (!token.empty()) {
        tokens.push_back(token);
        token.clear();
      }
      continue;
    }
    token += s[i];
  }
  if (!token.empty()) {
    tokens.push_back(token);
  }
  return tokens;
}

void parse_server_directive(ServerConfig &server,
                            const std::vector<std::string> &tokens) {
  if (tokens.empty()) {
    throw std::runtime_error("Empty server directive");
  }
  std::string directive = tokens[0];
  if (directive == "listen") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid listen directive");
    std::string listen_str = tokens[1];
    size_t colon_pos = listen_str.find(':');
    if (colon_pos != std::string::npos) {
      std::string inter = listen_str.substr(0, colon_pos);
      int port = ft_atoi(listen_str.substr(colon_pos + 1).c_str());
      if (port < 0 || port > 65535) {
        throw std::runtime_error("Invalid port number");
      }
      server.setInterPort(inter, port);
    } else {
      std::string inter = "0.0.0.0";
      int port = ft_atoi(listen_str.c_str());
      if (port < 0 || port > 65535) {
        throw std::runtime_error("Invalid port number");
      }
      server.setInterPort(inter, port);
    }
  } else if (directive == "server_name") {
    if (tokens.size() < 2)
      throw std::runtime_error("Invalid server_name directive");
    std::vector<std::string> names;
    for (size_t i = 1; i < tokens.size(); ++i) {
      names.push_back(tokens[i]);
    }
    server.setServerNames(names);
  } else if (directive == "root") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid root directive");
    server.setRoot(tokens[1]);
  } else if (directive == "index") {
    if (tokens.size() < 2)
      throw std::runtime_error("Invalid index directive");
    std::vector<std::string> indices;
    for (size_t i = 1; i < tokens.size(); ++i) {
      indices.push_back(tokens[i]);
    }
    server.setIndex(indices);
  } else if (directive == "error_page") {
    if (tokens.size() < 3)
      throw std::runtime_error("Invalid error_page directive");
    std::map<int, std::string> pages = server.getErrorPages();
    for (size_t i = 1; i < tokens.size() - 1; ++i) {
      int code = ft_atoi(tokens[i].c_str());
      if (code < 100 || code > 599) {
        std::stringstream ss;
        ss << code;
        throw std::runtime_error("Invalid error code: " + ss.str());
      }
      pages[code] = tokens[tokens.size() - 1];
    }
    server.setErrorPages(pages);
  } else if (directive == "client_max_body_size") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid client_max_body_size directive");
    std::string size_str = tokens[1];
    size_t multiplier = 1;
    if (!size_str.empty()) {
      char last_char = size_str[size_str.length() - 1];
      if (last_char == 'm' || last_char == 'M') {
        multiplier = 1024 * 1024;
        size_str = size_str.substr(0, size_str.length() - 1);
      } else if (last_char == 'k' || last_char == 'K') {
        multiplier = 1024;
        size_str = size_str.substr(0, size_str.length() - 1);
      }
    }
    server.setClientMaxBodySize(ft_atoi(size_str.c_str()) * multiplier);
  } else if (directive == "autoindex") {
    if (tokens.size() != 2 || (tokens[1] != "on" && tokens[1] != "off")) {
      throw std::runtime_error("Invalid autoindex directive");
    }
    server.setAutoindex(tokens[1] == "on");
  } else {
    throw std::runtime_error("Unknown server directive: " + directive);
  }
}

std::string to_Lower(const std::string &str) {
  std::string result = str;
  for (size_t i = 0; i < result.length(); ++i) {
    if (result[i] >= 'A' && result[i] <= 'Z') {
      result[i] += ('a' - 'A');
    }
  }
  return result;
}

bool validAllow(std::string token) {
  if (to_Lower(token) != "get" && to_Lower(token) != "post" &&
      to_Lower(token) != "delete")
    return (false);
  return (true);
}

void parse_location_directive(LocationConfig &location,
                              const std::vector<std::string> &tokens) {
  if (tokens.empty()) {
    throw std::runtime_error("Empty location directive");
  }
  std::string directive = tokens[0];
  if (directive == "allow") {
    if (tokens.size() < 2)
      throw std::runtime_error("Invalid allow directive");
    location.allowed_methods.clear();
    location.allowed_methods2.clear();
    for (size_t i = 1; i < tokens.size(); ++i) {
      if (!validAllow(tokens[i]))
        throw std::runtime_error("Invalid allow directive");
      if (to_Lower(tokens[i]) == "get")
        location.allowed_methods2.push_back(GET);
      else if (to_Lower(tokens[i]) == "post")
        location.allowed_methods2.push_back(POST);
      else if (to_Lower(tokens[i]) == "delete")
        location.allowed_methods2.push_back(DELETE);
      location.allowed_methods.push_back(tokens[i]);
    }
  } else if (directive == "root") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid root directive");
    location.root = tokens[1];
  } else if (directive == "alias") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid alias directive");
    location.alias = tokens[1];
  } else if (directive == "index") {
    if (tokens.size() < 2)
      throw std::runtime_error("Invalid index directive");
    location.index.clear();
    for (size_t i = 1; i < tokens.size(); ++i) {
      location.index.push_back(tokens[i]);
    }
  } else if (directive == "return") {
    if (tokens.size() != 3)
      throw std::runtime_error("Invalid return directive");
    location.redirect_code = ft_atoi(tokens[1].c_str());
    if (location.redirect_code < 300 || location.redirect_code > 399) {
      std::stringstream ss;
      ss << location.redirect_code;
      throw std::runtime_error("Invalid redirect code: " + ss.str());
    }
    location.redirect_url = tokens[2];
  } else if (directive == "autoindex") {
    if (tokens.size() != 2 || (tokens[1] != "on" && tokens[1] != "off")) {
      throw std::runtime_error("Invalid autoindex directive");
    }
    location.autoindex = (tokens[1] == "on");
  } else if (directive == "upload_store") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid upload_store directive");
    location.upload_store = tokens[1];
  } else if (directive == "cgi_ext") {
    if (tokens.size() != 3)
      throw std::runtime_error(
          "Invalid cgi_ext directive: must specify extension and binary path");
    if (!tokens[1].empty() && tokens[1][0] != '.') {
      throw std::runtime_error("CGI extension must start with a dot");
    }
    if (tokens[2].empty()) {
      throw std::runtime_error("CGI binary path cannot be empty");
    }
    location.cgi_ext[tokens[1]] = tokens[2];
  } else {
    throw std::runtime_error("Unknown location directive: " + directive);
  }
}

std::vector<ServerConfig> parseConfig(const std::string &file) {
  std::ifstream ifs(file.c_str());
  if (!ifs.is_open()) {
    throw std::runtime_error("Cannot open config file: " + file);
  }

  std::vector<ServerConfig> configs;
  std::string line;
  ServerConfig current_server;
  LocationConfig current_location;
  bool in_server = false, in_location = false;
  bool has_server_block = false;

  while (std::getline(ifs, line)) {
    line = trim(line);
    if (line.empty() || line[0] == '#')
      continue;

    if (!line.empty() && line[line.length() - 1] == '{') {
      std::vector<std::string> tokens =
          split_with_quotes(line.substr(0, line.length() - 1));
      if (tokens.size() == 1 && tokens[0] == "server") {
        if (in_server)
          throw std::runtime_error("Nested server blocks not allowed");
        in_server = true;
        has_server_block = true;
        current_server = ServerConfig();
      } else if (tokens[0] == "location") {
        if (!in_server)
          throw std::runtime_error("Location outside of server block");
        LocationConfig new_location;
        if (tokens.size() == 2) {
          new_location.path = tokens[1];
          if (new_location.path.size() < 1 || new_location.path[0] != '/')
            throw std::runtime_error("Invalid location directive");
        } else if (tokens.size() == 3 && tokens[1] == "=") {
          if (tokens[2].size() < 1 || tokens[2][0] != '/')
            throw std::runtime_error("Invalid location directive");
          new_location.path = "= " + tokens[2];
        } else if (tokens.size() == 3 && tokens[1] == "^~") {
          if (tokens[2].size() < 1 || tokens[2][0] != '/')
            throw std::runtime_error("Invalid location directive");
          new_location.path = "^~ " + tokens[2];
        } else {
          throw std::runtime_error(
              "Invalid location directive (regex not allowed): " + line);
        }

        new_location.redirect_code = 0; // Default
        new_location.autoindex = current_server.isAutoindex();
        new_location.index = current_server.getIndex();
        new_location.root = current_server.getRoot();
        current_server.addLocation(new_location);
        in_location = true;
      } else {
        throw std::runtime_error("Invalid block starter: " + line);
      }
    } else if (line == "}") {
      if (in_location) {
        in_location = false;
      } else if (in_server) {
        // Validate mandatory directives
        if (current_server.getInterPort().empty())
          throw std::runtime_error("Missing mandatory listen directive");
        if (!current_server.hasRoot())
          throw std::runtime_error("Missing mandatory root directive");
        configs.push_back(current_server);
        in_server = false;
      } else {
        throw std::runtime_error("Unmatched closing brace");
      }
    } else if (!line.empty() && line[line.length() - 1] == ';') {
      std::string directive_line = line.substr(0, line.length() - 1);
      std::vector<std::string> tokens = split_with_quotes(directive_line);
      if (tokens.empty()) {
        throw std::runtime_error("Empty directive line");
      }
      if (in_location) {
        parse_location_directive(
            const_cast<LocationConfig &>(current_server.getLocations().back()),
            tokens);
      } else if (in_server) {
        parse_server_directive(current_server, tokens);
      } else {
        throw std::runtime_error("Directives must be inside server block");
      }
    } else {
      throw std::runtime_error("Invalid line: " + line);
    }
  }

  if (in_server || in_location) {
    throw std::runtime_error("Unclosed block in config file");
  }

  if (!has_server_block) {
    throw std::runtime_error("No server block found in config file");
  }

  ifs.close();
  return configs;
}

bool endsWith(const std::string &str, const std::string &suffix) {
  if (suffix.length() > str.length()) {
    return false;
  }
  return str.substr(str.length() - suffix.length()) == suffix;
}

bool isPathCompatible(const std::string &locationPath,
                      const std::string &requestedPath) {
  if (locationPath.empty() || requestedPath.empty()) {
    return false;
  }

  std::string locPath = trim(locationPath);
  std::string reqPath = trim(requestedPath);
  if (locPath.empty() || reqPath.empty()) {
    return false;
  }

  if (locPath.length() >= 2 && locPath.substr(0, 2) == "= ") {
    std::string exactPath = locPath.substr(2);
    return reqPath == exactPath;
  }

  if (locPath.length() >= 3 && locPath.substr(0, 3) == "^~ ") {
    std::string prefix = locPath.substr(3);
    if (reqPath.length() >= prefix.length()) {
      return reqPath.substr(0, prefix.length()) == prefix;
    }
    return false;
  }

  if (reqPath.length() >= locPath.length()) {
    return reqPath.substr(0, locPath.length()) == locPath;
  }
  return false;
}
