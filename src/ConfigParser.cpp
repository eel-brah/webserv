/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#             */
/*   Updated: 2025/08/10 15:15:16 by muel-bak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ConfigParser.hpp"
#include "../include/helpers.hpp"


bool safeAtoi(const std::string &str, long &result) {
  if (str.empty()) return false;
  result = 0;
  bool is_negative = false;
  size_t i = 0;

  if (str[0] == '-') {
    is_negative = true;
    ++i;
  }

  for (; i < str.length(); ++i) {
    if (str[i] < '0' || str[i] > '9') return false;
    result = result * 10 + (str[i] - '0');
    if (result > 2147483647L || (is_negative && result > 2147483648L)) {
      return false;
    }
  }
  if (is_negative) result = -result;
  return true;
}

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
  if (directive.length() > MAX_STRING_LENGTH) {
    throw std::runtime_error("Directive name too long: " + directive);
  }

  if (directive == "listen") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid listen directive");
    std::string listen_str = tokens[1];
    if (listen_str.length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("Listen value too long");
    }
    size_t colon_pos = listen_str.find(':');
    if (colon_pos != std::string::npos) {
      std::string inter = listen_str.substr(0, colon_pos);
      if (inter.length() > MAX_STRING_LENGTH) {
        throw std::runtime_error("Interface name too long");
      }
      std::string port_str = listen_str.substr(colon_pos + 1);
      long port;
      if (!safeAtoi(port_str, port) || port < 0 || port > MAX_PORT) {
        throw std::runtime_error("Invalid port number: " + port_str);
      }
      server.setInterPort(inter, static_cast<int>(port));
    } else {
      std::string inter = "0.0.0.0";
      long port;
      if (!safeAtoi(listen_str, port) || port < 0 || port > MAX_PORT) {
        throw std::runtime_error("Invalid port number: " + listen_str);
      }
      server.setInterPort(inter, static_cast<int>(port));
    }
  } else if (directive == "server_name") {
    if (tokens.size() < 2)
      throw std::runtime_error("Invalid server_name directive");
    std::vector<std::string> names;
    for (size_t i = 1; i < tokens.size(); ++i) {
      if (tokens[i].length() > MAX_STRING_LENGTH) {
        throw std::runtime_error("Server name too long: " + tokens[i]);
      }
      if (tokens[i].empty()) {
        throw std::runtime_error("Empty server name not allowed");
      }
      names.push_back(tokens[i]);
    }
    if (names.size() > MAX_VECTOR_SIZE) {
      throw std::runtime_error("Too many server names");
    }
    server.setServerNames(names);
  } else if (directive == "root") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid root directive");
    if (tokens[1].length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("Root path too long");
    }
    if (tokens[1].empty()) {
      throw std::runtime_error("Empty root path not allowed");
    }
    server.setRoot(tokens[1]);
  } else if (directive == "index") {
    if (tokens.size() < 2)
      throw std::runtime_error("Invalid index directive");
    std::vector<std::string> indices;
    for (size_t i = 1; i < tokens.size(); ++i) {
      if (tokens[i].length() > MAX_STRING_LENGTH) {
        throw std::runtime_error("Index file name too long: " + tokens[i]);
      }
      if (tokens[i].empty()) {
        throw std::runtime_error("Empty index file name not allowed");
      }
      indices.push_back(tokens[i]);
    }
    if (indices.size() > MAX_VECTOR_SIZE) {
      throw std::runtime_error("Too many index files");
    }
    server.setIndex(indices);
  } else if (directive == "error_page") {
    if (tokens.size() < 3)
      throw std::runtime_error("Invalid error_page directive");
    std::map<int, std::string> pages = server.getErrorPages();
    if (pages.size() >= MAX_MAP_SIZE) {
      throw std::runtime_error("Too many error pages");
    }
    for (size_t i = 1; i < tokens.size() - 1; ++i) {
      long code;
      if (!safeAtoi(tokens[i], code) || code < MIN_ERROR_CODE || code > MAX_ERROR_CODE) {
        throw std::runtime_error("Invalid error code: " + tokens[i]);
      }
      if (tokens[tokens.size() - 1].length() > MAX_STRING_LENGTH) {
        throw std::runtime_error("Error page path too long");
      }
      if (tokens[tokens.size() - 1].empty()) {
        throw std::runtime_error("Empty error page path not allowed");
      }
      pages[static_cast<int>(code)] = tokens[tokens.size() - 1];
    }
    server.setErrorPages(pages);
  } else if (directive == "client_max_body_size") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid client_max_body_size directive");
    std::string size_str = tokens[1];
    if (size_str.length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("Client max body size value too long");
    }
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
    long size;
    if (!safeAtoi(size_str, size) || size < 0 || size > static_cast<long>(MAX_BODY_SIZE / multiplier)) {
      throw std::runtime_error("Invalid client_max_body_size value: " + tokens[1]);
    }
    server.setClientMaxBodySize(static_cast<size_t>(size) * multiplier);
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
  if (directive.length() > MAX_STRING_LENGTH) {
    throw std::runtime_error("Directive name too long: " + directive);
  }

  if (directive == "allow") {
    if (tokens.size() < 2)
      throw std::runtime_error("Invalid allow directive");
    if (tokens.size() - 1 > MAX_VECTOR_SIZE) {
      throw std::runtime_error("Too many allowed methods");
    }
    location.allowed_methods.clear();
    location.allowed_methods2.clear();
    for (size_t i = 1; i < tokens.size(); ++i) {
      if (tokens[i].length() > MAX_STRING_LENGTH) {
        throw std::runtime_error("Allowed method too long: " + tokens[i]);
      }
      if (!validAllow(tokens[i]))
        throw std::runtime_error("Invalid allow directive: " + tokens[i]);
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
    if (tokens[1].length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("Root path too long");
    }
    if (tokens[1].empty()) {
      throw std::runtime_error("Empty root path not allowed");
    }
    location.root = tokens[1];
  } else if (directive == "alias") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid alias directive");
    if (tokens[1].length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("Alias path too long");
    }
    if (tokens[1].empty()) {
      throw std::runtime_error("Empty alias path not allowed");
    }
    location.alias = tokens[1];
  } else if (directive == "index") {
    if (tokens.size() < 2)
      throw std::runtime_error("Invalid index directive");
    if (tokens.size() - 1 > MAX_VECTOR_SIZE) {
      throw std::runtime_error("Too many index files");
    }
    location.index.clear();
    for (size_t i = 1; i < tokens.size(); ++i) {
      if (tokens[i].length() > MAX_STRING_LENGTH) {
        throw std::runtime_error("Index file name too long: " + tokens[i]);
      }
      if (tokens[i].empty()) {
        throw std::runtime_error("Empty index file name not allowed");
      }
      location.index.push_back(tokens[i]);
    }
  } else if (directive == "return") {
    if (tokens.size() != 3)
      throw std::runtime_error("Invalid return directive");
    long code;
    if (!safeAtoi(tokens[1], code) || code < MIN_REDIRECT_CODE || code > MAX_REDIRECT_CODE) {
      throw std::runtime_error("Invalid redirect code: " + tokens[1]);
    }
    if (tokens[2].length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("Redirect URL too long");
    }
    if (tokens[2].empty()) {
      throw std::runtime_error("Empty redirect URL not allowed");
    }
    location.redirect_code = static_cast<int>(code);
    location.redirect_url = tokens[2];
  } else if (directive == "autoindex") {
    if (tokens.size() != 2 || (tokens[1] != "on" && tokens[1] != "off")) {
      throw std::runtime_error("Invalid autoindex directive");
    }
    location.autoindex = (tokens[1] == "on");
  } else if (directive == "upload_store") {
    if (tokens.size() != 2)
      throw std::runtime_error("Invalid upload_store directive");
    if (tokens[1].length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("Upload store path too long");
    }
    if (tokens[1].empty()) {
      throw std::runtime_error("Empty upload store path not allowed");
    }
    location.upload_store = tokens[1];
  } else if (directive == "cgi_ext") {
    if (tokens.size() != 3)
      throw std::runtime_error(
          "Invalid cgi_ext directive: must specify extension and binary path");
    if (location.cgi_ext.size() >= MAX_MAP_SIZE) {
      throw std::runtime_error("Too many CGI extensions");
    }
    if (!tokens[1].empty() && tokens[1][0] != '.') {
      throw std::runtime_error("CGI extension must start with a dot");
    }
    if (tokens[1].length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("CGI extension too long");
    }
    if (tokens[2].length() > MAX_STRING_LENGTH) {
      throw std::runtime_error("CGI binary path too long");
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

  // Check file size
  ifs.seekg(0, std::ios::end);
  size_t file_size = ifs.tellg();
  ifs.seekg(0, std::ios::beg);
  if (file_size > 1024 * 1024 * 10) {
    ifs.close();
    throw std::runtime_error("Config file too large");
  }
  if (file_size == 0) {
    ifs.close();
    throw std::runtime_error("Config file is empty");
  }

  std::vector<ServerConfig> configs;
  std::string line;
  ServerConfig current_server;
  LocationConfig current_location;
  bool in_server = false, in_location = false;
  bool has_server_block = false;

  while (std::getline(ifs, line))
  {
    if (ifs.fail() || ifs.bad())
                throw std::runtime_error("Error reading from file: stream failure");
    if (line.length() > MAX_STRING_LENGTH) {
      ifs.close();
      throw std::runtime_error("Config line too long");
    }
    line = trim(line);
    if (line.empty() || line[0] == '#')
      continue;

    if (!line.empty() && line[line.length() - 1] == '{') {
      std::vector<std::string> tokens =
          split_with_quotes(line.substr(0, line.length() - 1));
      if (tokens.size() == 1 && tokens[0] == "server") {
        if (in_server)
          throw std::runtime_error("Nested server blocks not allowed");
        if (configs.size() >= MAX_VECTOR_SIZE) {
          ifs.close();
          throw std::runtime_error("Too many server blocks");
        }
        in_server = true;
        has_server_block = true;
        current_server = ServerConfig();
      } else if (tokens[0] == "location") {
        if (!in_server)
          throw std::runtime_error("Location outside of server block");
        LocationConfig new_location;
        if (tokens.size() == 2) {
          if (tokens[1].length() > MAX_STRING_LENGTH) {
            ifs.close();
            throw std::runtime_error("Location path too long");
          }
          new_location.path = tokens[1];
          if (new_location.path.size() < 1 || new_location.path[0] != '/')
            throw std::runtime_error("Invalid location directive: " + new_location.path);
        } else if (tokens.size() == 3 && tokens[1] == "=") {
          if (tokens[2].length() > MAX_STRING_LENGTH) {
            ifs.close();
            throw std::runtime_error("Location path too long");
          }
          if (tokens[2].size() < 1 || tokens[2][0] != '/')
            throw std::runtime_error("Invalid location directive: " + tokens[2]);
          new_location.path = "= " + tokens[2];
        } else if (tokens.size() == 3 && tokens[1] == "^~") {
          if (tokens[2].length() > MAX_STRING_LENGTH) {
            ifs.close();
            throw std::runtime_error("Location path too long");
          }
          if (tokens[2].size() < 1 || tokens[2][0] != '/')
            throw std::runtime_error("Invalid location directive: " + tokens[2]);
          new_location.path = "^~ " + tokens[2];
        } else {
          ifs.close();
          throw std::runtime_error(
              "Invalid location directive (regex not allowed): " + line);
        }

        new_location.redirect_code = 0; // Default
        new_location.autoindex = current_server.isAutoindex();
        new_location.index = current_server.getIndex();
        new_location.root = current_server.getRoot();
        if (current_server.getLocations().size() >= MAX_VECTOR_SIZE) {
          ifs.close();
          throw std::runtime_error("Too many location blocks");
        }
        current_server.addLocation(new_location);
        in_location = true;
      } else {
        ifs.close();
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
        ifs.close();
        throw std::runtime_error("Unmatched closing brace");
      }
    } else if (!line.empty() && line[line.length() - 1] == ';') {
      std::string directive_line = line.substr(0, line.length() - 1);
      std::vector<std::string> tokens = split_with_quotes(directive_line);
      if (tokens.empty()) {
        ifs.close();
        throw std::runtime_error("Empty directive line");
      }
      if (in_location) {
        parse_location_directive(
            const_cast<LocationConfig &>(current_server.getLocations().back()),
            tokens);
      } else if (in_server) {
        parse_server_directive(current_server, tokens);
      } else {
        ifs.close();
        throw std::runtime_error("Directives must be inside server block");
      }
    } else {
      ifs.close();
      throw std::runtime_error("Invalid line: " + line);
    }
  }

  if (in_server || in_location) {
    ifs.close();
    throw std::runtime_error("Unclosed block in config file");
  }

  if (!has_server_block) {
    ifs.close();
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
