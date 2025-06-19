/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.cpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#             */
/*   Updated: 2025/06/19 14:13:23 by muel-bak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

// Global configuration struct to store top-level directives
struct GlobalConfig {
    int worker_processes;
    std::string pid;
    int worker_connections; // From events block
    std::string include; // e.g., mime.types
    std::string default_type;
};

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

std::string remove_comments(const std::string& str) {
    size_t comment_pos = str.find('#');
    if (comment_pos != std::string::npos) {
        return str.substr(0, comment_pos);
    }
    return str;
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

void parse_global_directive(GlobalConfig& global, const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Empty global directive");
    }
    std::string directive = tokens[0];
    if (directive == "worker_processes") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid worker_processes directive");
        global.worker_processes = std::atoi(tokens[1].c_str());
    } else if (directive == "pid") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid pid directive");
        global.pid = tokens[1];
    } else if (directive == "include") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid include directive");
        global.include = tokens[1];
    } else if (directive == "default_type") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid default_type directive");
        global.default_type = tokens[1];
    } else {
        throw std::runtime_error("Unknown global directive: " + directive);
    }
}

void parse_events_directive(GlobalConfig& global, const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        throw std::runtime_error("Empty events directive");
    }
    std::string directive = tokens[0];
    if (directive == "worker_connections") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid worker_connections directive");
        global.worker_connections = std::atoi(tokens[1].c_str());
    } else {
        throw std::runtime_error("Unknown events directive: " + directive);
    }
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
    } else if (directive == "client_max_body_size") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid client_max_body_size directive");
        server.client_max_body_size = std::atoi(tokens[1].c_str());
    } else if (directive == "error_page") {
        if (tokens.size() < 3) throw std::runtime_error("Invalid error_page directive");
        int code = std::atoi(tokens[1].c_str());
        server.error_pages[code] = tokens[2];
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
    } else if (directive == "redirect") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid redirect directive");
        location.redirect = tokens[1];
    } else if (directive == "autoindex") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid autoindex directive");
        location.autoindex = (tokens[1] == "on");
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
    } else if (directive == "upload_path") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid upload_path directive");
        location.upload_path = tokens[1];
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
    GlobalConfig global = {0, "", 0, "", ""};
    std::string line;
    ServerConfig current_server;
    LocationConfig current_location;
    std::vector<LocationConfig*> location_stack;
    bool in_events = false, in_http = false, in_server = false, in_location = false;

    while (std::getline(ifs, line)) {
        line = remove_comments(line); // Remove comments first
        line = trim(line);
        if (line.empty()) continue;

        if (!line.empty() && line[line.length() - 1] == '{') {
            std::vector<std::string> tokens = split_with_quotes(line.substr(0, line.length() - 1));
            if (tokens.size() == 1 && tokens[0] == "events") {
                if (in_events || in_http || in_server || in_location) {
                    throw std::runtime_error("Misplaced events block");
                }
                in_events = true;
            } else if (tokens.size() == 1 && tokens[0] == "http") {
                if (in_events || in_http || in_server || in_location) {
                    throw std::runtime_error("Misplaced http block");
                }
                in_http = true;
            } else if (tokens.size() == 1 && tokens[0] == "server") {
                if (!in_http) throw std::runtime_error("Server block outside http block");
                if (in_server) throw std::runtime_error("Nested server blocks not allowed");
                in_server = true;
                current_server = ServerConfig();
                current_server.host = "0.0.0.0";
                current_server.port = 80;
                current_server.client_max_body_size = 1048576; // Default 1MB
            } else if (tokens[0] == "location") {
                if (!in_server && !in_location) throw std::runtime_error("Location outside of server or location block");
                LocationConfig new_location;
                if (tokens.size() == 2) {
                    new_location.path = tokens[1];
                } else if (tokens.size() == 3 && tokens[1] == "=") {
                    new_location.path = "= " + tokens[2];
                } else {
                    throw std::runtime_error("Invalid location directive: " + line);
                }
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
            } else if (in_http) {
                in_http = false;
            } else if (in_events) {
                in_events = false;
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
            } else if (in_events) {
                parse_events_directive(global, tokens);
            } else if (in_http) {
                parse_global_directive(global, tokens);
            } else {
                parse_global_directive(global, tokens);
            }
        } else {
            throw std::runtime_error("Invalid line: " + line);
        }
    }

    if (in_events || in_http || in_server || in_location) {
        throw std::runtime_error("Unclosed block in config file");
    }

    ifs.close();
    return configs;
}

// Path matching functions

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

bool endsWithAnyCaseInsensitive(const std::string& str, const std::string& suffixes) {
    std::string lowerStr = toLower(str);
    std::string lowerSuffixes = toLower(suffixes);
    if (lowerSuffixes[0] != '.' || lowerSuffixes[lowerSuffixes.length() - 1] != '$') {
        return false;
    }
    std::string ext = lowerSuffixes.substr(1, lowerSuffixes.length() - 2);
    return endsWith(lowerStr, "." + ext);
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

    if (reqPath.length() >= locPath.length()) {
        return reqPath.substr(0, locPath.length()) == locPath;
    }
    return false;
}