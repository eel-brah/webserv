/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.42.fr>          +#           */
/*                                                +#           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          ##             */
/*   Updated: 2025/06/21 16:08:22 by muel-bak         ###   .fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

std::string trim(const std::string& str) {
    size_t start = 0;
    while (start < str.length() && (str[start] == ' ' || str[start] == '\t' || str[start] == '\n' || str[start] == '\r')) {
        ++start;
    }
    if (start == str.length()) {
        return "";
    }
    size_t end = str.length() - 1;
    while (end > start && (str[end] == ' ' || str[end] == '\t' || str[end] == '\n' || str[end] == '\r')) {
        --end;
    }
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split_with_quotes(const std::string& s) {
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

void parse_server_directive(ServerConfig& server, const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Empty server directive");
    }
    std::string directive = tokens[0];
    if (directive == "listen") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid listen directive");
        std::string listen_str = tokens[1];
        size_t colon_pos = listen_str.find(':');
        if (colon_pos != std::string::npos) {
            server.setHost(listen_str.substr(0, colon_pos));
            server.setPort(std::atoi(listen_str.substr(colon_pos + 1).c_str()));
        } else {
            server.setHost("0.0.0.0");
            server.setPort(std::atoi(listen_str.c_str()));
        }
    } else if (directive == "server_name") {
        if (tokens.size() < 2) throw std::runtime_error("Invalid server_name directive");
        std::vector<std::string> names;
        for (size_t i = 1; i < tokens.size(); ++i) {
            names.push_back(tokens[i]);
        }
        server.setServerNames(names);
    } else if (directive == "root") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid root directive");
        server.setRoot(tokens[1]);
    } else if (directive == "index") {
        if (tokens.size() < 2) throw std::runtime_error("Invalid index directive");
        std::vector<std::string> indices;
        for (size_t i = 1; i < tokens.size(); ++i) {
            indices.push_back(tokens[i]);
        }
        server.setIndex(indices);
    } else if (directive == "error_page") {
        if (tokens.size() < 3) throw std::runtime_error("Invalid error_page directive");
        std::map<int, std::string> pages = server.getErrorPages();
        for (size_t i = 1; i < tokens.size() - 1; ++i) {
            int code = std::atoi(tokens[i].c_str());
            if (code < 100 || code > 599) {
                std::stringstream ss;
                ss << code;
                throw std::runtime_error("Invalid error code: " + ss.str());
            }
            pages[code] = tokens[tokens.size() - 1];
        }
        server.setErrorPages(pages);
    } else if (directive == "client_max_body_size") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid client_max_body_size directive");
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
        server.setClientMaxBodySize(std::atoi(size_str.c_str()) * multiplier);
    } else if (directive == "autoindex") {
        if (tokens.size() != 2 || (tokens[1] != "on" && tokens[1] != "off")) {
            throw std::runtime_error("Invalid autoindex directive");
        }
        server.setAutoindex(tokens[1] == "on");
    } else {
        throw std::runtime_error("Unknown server directive: " + directive);
    }
}

void parse_location_directive(LocationConfig& location, const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Empty location directive");
    }
    std::string directive = tokens[0];
    if (directive == "allow") {
        if (tokens.size() < 2) throw std::runtime_error("Invalid allow directive");
        location.allowed_methods.clear();
        for (size_t i = 1; i < tokens.size(); ++i) {
            // TODO: check if not valid
            if (tokens[i] == "GET")
              location.allowed_methods2.push_back(GET);
            else if (tokens[i] == "POST")
              location.allowed_methods2.push_back(POST);
            else if (tokens[i] == "DELETE")
              location.allowed_methods2.push_back(DELETE);
            location.allowed_methods.push_back(tokens[i]);
        }
    } else if (directive == "root") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid root directive");
        location.root = tokens[1];
    } else if (directive == "alias") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid alias directive");
        location.alias = tokens[1];
    } else if (directive == "try_files") {
        if (tokens.size() < 2) throw std::runtime_error("Invalid try_files directive");
        location.try_files.clear();
        for (size_t i = 1; i < tokens.size(); ++i) {
            location.try_files.push_back(tokens[i]);
        }
    } else if (directive == "proxy_pass") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid proxy_pass directive");
        location.proxy_pass = tokens[1];
    } else if (directive == "proxy_set_header") {
        if (tokens.size() != 3) throw std::runtime_error("Invalid proxy_set_header directive");
        location.proxy_set_headers[tokens[1]] = tokens[2];
    } else if (directive == "expires") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid expires directive");
        location.expires = tokens[1];
    } else if (directive == "access_log") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid access_log directive");
        location.access_log = tokens[1];
    } else if (directive == "auth_basic") {
        if (tokens.size() < 2) throw std::runtime_error("Invalid auth_basic directive");
        std::string value;
        for (size_t i = 1; i < tokens.size(); ++i) {
            value += tokens[i];
            if (i < tokens.size() - 1) value += " ";
        }
        if (!value.empty() && value[0] == '"' && value[value.length() - 1] == '"') {
            value = value.substr(1, value.length() - 2);
        }
        location.auth_basic = value;
    } else if (directive == "auth_basic_user_file") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid auth_basic_user_file directive");
        location.auth_basic_user_file = tokens[1];
    } else if (directive == "deny") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid deny directive");
        location.deny = tokens[1];
    } else if (directive == "index") {
        if (tokens.size() < 2) throw std::runtime_error("Invalid index directive");
        location.index.clear();
        for (size_t i = 1; i < tokens.size(); ++i) {
            location.index.push_back(tokens[i]);
        }
    } else if (directive == "cgi_ext") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid cgi_ext directive");
        location.cgi_ext = tokens[1];
    } else if (directive == "cgi_bin") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid cgi_bin directive");
        location.cgi_bin = tokens[1];
    } else if (directive == "return") {
        if (tokens.size() != 3) throw std::runtime_error("Invalid return directive");
        location.redirect_code = std::atoi(tokens[1].c_str());
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
        if (tokens.size() != 2) throw std::runtime_error("Invalid upload_store directive");
        location.upload_store = tokens[1];
    } else {
        throw std::runtime_error("Unknown location directive: " + directive);
    }
}

std::vector<ServerConfig> parseConfig(const std::string& file) {
    std::ifstream ifs(file.c_str());
    if (!ifs.is_open()) {
        throw std::runtime_error("Cannot open config file: " + file);
    }

    std::vector<ServerConfig> configs;
    std::string line;
    ServerConfig current_server;
    LocationConfig current_location;
    std::vector<LocationConfig*> location_stack;
    bool in_server = false, in_location = false;

    while (std::getline(ifs, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (!line.empty() && line[line.length() - 1] == '{') {
            std::vector<std::string> tokens = split_with_quotes(line.substr(0, line.length() - 1));
            if (tokens.size() == 1 && tokens[0] == "server") {
                if (in_server) throw std::runtime_error("Nested server blocks not allowed");
                in_server = true;
                current_server = ServerConfig();
            } else if (tokens[0] == "location") {
                if (!in_server && !in_location) throw std::runtime_error("Location outside of server or location block");
                LocationConfig new_location;
                if (tokens.size() == 2) {
                    new_location.path = tokens[1];
                } else if (tokens.size() == 3 && tokens[1] == "=") {
                    new_location.path = "= " + tokens[2];
                } else if (tokens.size() == 3 && tokens[1] == "^~") {
                    new_location.path = "^~ " + tokens[2];
                } else {
                    throw std::runtime_error("Invalid location directive (regex not allowed): " + line);
                }
                new_location.redirect_code = 0; // Default
                new_location.autoindex = false; // Default
                if (in_location) {
                    location_stack.back()->nested_locations.push_back(new_location);
                    location_stack.push_back(&location_stack.back()->nested_locations.back());
                } else {
                    current_server.addLocation(new_location);
                    // Use a temporary vector to get a non-const pointer
                    location_stack.push_back(const_cast<LocationConfig*>(&current_server.getLocations().back()));
                }
                in_location = true;
            } else {
                throw std::runtime_error("Invalid block starter: " + line);
            }
        } else if (line == "}") {
            if (in_location) {
                location_stack.pop_back();
                in_location = !location_stack.empty();
            } else if (in_server) {
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
                parse_location_directive(*location_stack.back(), tokens);
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

    ifs.close();
    return configs;
}

std::string toLower(const std::string& str) {
    std::string result = str;
    for (size_t i = 0; i < result.length(); ++i) {
        if (result[i] >= 'A' && result[i] <= 'Z') {
            result[i] += ('a' - 'A');
        }
    }
    return result;
}

bool endsWith(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

bool isPathCompatible(const std::string& locationPath, const std::string& requestedPath) {
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
