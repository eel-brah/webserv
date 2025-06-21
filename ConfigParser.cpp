/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#             */
/*   Updated: 2025/06/21 16:08:22 by muel-bak         ###   ########.fr       */
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
            server.host = listen_str.substr(0, colon_pos);
            server.port = std::atoi(listen_str.substr(colon_pos + 1).c_str());
        } else {
            server.host = "0.0.0.0";
            server.port = std::atoi(listen_str.c_str());
        }
    } else if (directive == "server_name") {
        if (tokens.size() < 2) throw std::runtime_error("Invalid server_name directive");
        for (size_t i = 1; i < tokens.size(); ++i) {
            server.server_names.push_back(tokens[i]);
        }
    } else if (directive == "root") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid root directive");
        server.root = tokens[1];
    } else if (directive == "index") {
        if (tokens.size() < 2) throw std::runtime_error("Invalid index directive");
        server.index.clear();
        for (size_t i = 1; i < tokens.size(); ++i) {
            server.index.push_back(tokens[i]);
        }
    } else if (directive == "error_page") {
        if (tokens.size() < 3) throw std::runtime_error("Invalid error_page directive");
        for (size_t i = 1; i < tokens.size() - 1; ++i) {
            int code = std::atoi(tokens[i].c_str());
            if (code < 100 || code > 599) throw std::runtime_error("Invalid error code: " + tokens[i]);
            server.error_pages[code] = tokens[tokens.size() - 1];
        }
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
        server.client_max_body_size = std::atoi(size_str.c_str()) * multiplier;
    } else if (directive == "autoindex") {
        if (tokens.size() != 2 || (tokens[1] != "on" && tokens[1] != "off")) {
            throw std::runtime_error("Invalid autoindex directive");
        }
        server.autoindex = (tokens[1] == "on");
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
        for (size_t i = 1; i < tokens.size(); ++i) {
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
            throw std::runtime_error("Invalid redirect code: " + tokens[1]);
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
                current_server.host = "0.0.0.0";
                current_server.port = 0;
                current_server.client_max_body_size = 0; // Default
                current_server.autoindex = true;
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
                    current_server.locations.push_back(new_location);
                    location_stack.push_back(&current_server.locations.back());
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

std::vector<ServerConfig> generateDefaultConfig() {
    std::vector<ServerConfig> configs;
    ServerConfig default_server;

    // Default server settings
    default_server.host = "0.0.0.0";
    default_server.port = 80;
    default_server.server_names.push_back("localhost");
    default_server.client_max_body_size = 1048576; // 1MB
    default_server.root = "/var/www/html";
    default_server.index.push_back("index.html");
    default_server.index.push_back("index.htm");
    default_server.autoindex = false;
    default_server.error_pages[404] = "/404.html";
    default_server.error_pages[500] = "/404.html";

    // Default location for "/"
    LocationConfig default_location;
    default_location.path = "/";
    default_location.allowed_methods.push_back("GET");
    default_location.allowed_methods.push_back("POST");
    default_location.root = "/var/www/html";
    default_location.index.push_back("index.html");
    default_location.index.push_back("index.htm");
    default_location.autoindex = false;
    default_location.redirect_code = 0; // No redirect

    // Default location for "/upload/"
    LocationConfig upload_location;
    upload_location.path = "/upload/";
    upload_location.allowed_methods.push_back("POST");
    upload_location.upload_store = "/tmp/uploads";
    upload_location.root = "/var/www/html";
    upload_location.autoindex = false;
    upload_location.redirect_code = 0;

    // Default location for "/php/"
    LocationConfig php_location;
    php_location.path = "/php/";
    php_location.allowed_methods.push_back("GET");
    php_location.allowed_methods.push_back("POST");
    php_location.root = "/var/www/php";
    php_location.cgi_ext = ".php";
    php_location.cgi_bin = "/usr/bin/php-cgi";
    php_location.autoindex = false;
    php_location.redirect_code = 0;

    // Add locations to server
    default_server.locations.push_back(default_location);
    default_server.locations.push_back(upload_location);
    default_server.locations.push_back(php_location);

    // Add server to configs
    configs.push_back(default_server);

    return configs;
}