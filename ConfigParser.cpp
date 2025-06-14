#include "ConfigParser.hpp"


struct GlobalConfig {
    int worker_processes;
    std::string pid;
    int worker_connections;
    std::string include;
    std::string default_type;
};

std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\n\r");
    return str.substr(start, end - start + 1);
}

std::vector<std::string> split_with_quotes(const std::string& s) {
    std::vector<std::string> tokens;
    std::string token;
    bool in_quotes = false;
    std::string::const_iterator it = s.begin();

    while (it != s.end()) {
        if (*it == '"') {
            in_quotes = !in_quotes;
            token += *it;
            ++it;
            continue;
        }
        if (!in_quotes && (*it == ' ' || *it == '\t')) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            ++it;
            continue;
        }
        token += *it;
        ++it;
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}

void parse_global_directive(GlobalConfig& global, const std::vector<std::string>& tokens) {
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
    std::string directive = tokens[0];
    if (directive == "worker_connections") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid worker_connections directive");
        global.worker_connections = std::atoi(tokens[1].c_str());
    } else {
        throw std::runtime_error("Unknown events directive: " + directive);
    }
}

void parse_server_directive(ServerConfig& server, const std::vector<std::string>& tokens) {
    std::string directive = tokens[0];
    if (directive == "listen") {
        if (tokens.size() != 2 && tokens.size() != 3) throw std::runtime_error("Invalid listen directive");
        std::string listen_str = tokens[1];
        size_t colon_pos = listen_str.find(':');
        if (colon_pos != std::string::npos) {
            server.host = listen_str.substr(0, colon_pos);
            server.port = std::atoi(listen_str.substr(colon_pos + 1).c_str());
        } else {
            server.host = "0.0.0.0";
            server.port = std::atoi(listen_str.c_str());
        }
        if (tokens.size() == 3 && tokens[2] == "ssl") {
            server.ssl = true;
        }
    } else if (directive == "server_name") {
        for (size_t i = 1; i < tokens.size(); ++i) {
            server.server_names.push_back(tokens[i]);
        }
    } else if (directive == "root") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid root directive");
        server.root = tokens[1];
    } else if (directive == "index") {
        for (size_t i = 1; i < tokens.size(); ++i) {
            server.index.push_back(tokens[i]);
        }
    } else if (directive == "ssl_certificate") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid ssl_certificate directive");
        server.ssl_certificate = tokens[1];
    } else if (directive == "ssl_certificate_key") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid ssl_certificate_key directive");
        server.ssl_certificate_key = tokens[1];
    } else {
        throw std::runtime_error("Unknown server directive: " + directive);
    }
}

void parse_location_directive(LocationConfig& location, const std::vector<std::string>& tokens) {
    std::string directive = tokens[0];
    if (directive == "allow") {
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
        // Remove quotes if present
        if (!value.empty() && value[0] == '"' && value[value.size() - 1] == '"') {
            value = value.substr(1, value.size() - 2);
        }
        location.auth_basic = value;
    } else if (directive == "auth_basic_user_file") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid auth_basic_user_file directive");
        location.auth_basic_user_file = tokens[1];
    } else if (directive == "deny") {
        if (tokens.size() != 2) throw std::runtime_error("Invalid deny directive");
        location.deny = tokens[1];
    } else if (directive == "index") {
        for (size_t i = 1; i < tokens.size(); ++i) {
            location.index.push_back(tokens[i]);
        }
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
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;

        if (!line.empty() && line[line.size() - 1] == '{') {
            std::vector<std::string> tokens = split_with_quotes(line.substr(0, line.size() - 1));
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
                current_server.port = 0;
            } else if (tokens[0] == "location") {
                if (!in_server && !in_location) throw std::runtime_error("Location outside of server or location block");
                LocationConfig new_location;
                if (tokens.size() == 2) {
                    new_location.path = tokens[1];
                } else if (tokens.size() == 3 && tokens[1] == "~") {
                    new_location.path = "~ " + tokens[2];
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
        } else if (!line.empty() && line[line.size() - 1] == ';') {
            std::string directive_line = line.substr(0, line.size() - 1);
            std::vector<std::string> tokens = split_with_quotes(directive_line);
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