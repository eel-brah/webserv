/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/14 18:33:40 by muel-bak          #+#    #+#             */
/*   Updated: 2025/06/14 18:33:43 by muel-bak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "ConfigParser.hpp"

void print_location(const LocationConfig& loc, int indent = 4) {
    for (int i = 0; i < indent; ++i) std::cout << " ";
    std::cout << "Path: " << loc.path << "\n";
    for (int i = 0; i < indent; ++i) std::cout << " ";
    std::cout << "  Allowed Methods: ";
    for (size_t k = 0; k < loc.allowed_methods.size(); ++k) {
        std::cout << loc.allowed_methods[k] << " ";
    }
    std::cout << "\n";
    for (int i = 0; i < indent; ++i) std::cout << " ";
    std::cout << "  Root: " << loc.root << "\n";
    if (!loc.index.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Index: ";
        for (size_t k = 0; k < loc.index.size(); ++k) {
            std::cout << loc.index[k] << " ";
        }
        std::cout << "\n";
    }
    if (!loc.alias.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Alias: " << loc.alias << "\n";
    }
    if (!loc.try_files.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Try Files: ";
        for (size_t k = 0; k < loc.try_files.size(); ++k) {
            std::cout << loc.try_files[k] << " ";
        }
        std::cout << "\n";
    }
    if (!loc.proxy_pass.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Proxy Pass: " << loc.proxy_pass << "\n";
    }
    if (!loc.proxy_set_headers.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Proxy Set Headers:\n";
        for (std::map<std::string, std::string>::const_iterator it = loc.proxy_set_headers.begin();
             it != loc.proxy_set_headers.end(); ++it) {
            for (int i = 0; i < indent + 2; ++i) std::cout << " ";
            std::cout << it->first << " -> " << it->second << "\n";
        }
    }
    if (!loc.expires.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Expires: " << loc.expires << "\n";
    }
    if (!loc.access_log.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Access Log: " << loc.access_log << "\n";
    }
    if (!loc.auth_basic.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Auth Basic: " << loc.auth_basic << "\n";
    }
    if (!loc.auth_basic_user_file.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Auth Basic User File: " << loc.auth_basic_user_file << "\n";
    }
    if (!loc.deny.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Deny: " << loc.deny << "\n";
    }
    if (!loc.cgi_ext.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  CGI: " << loc.cgi_ext << " -> " << loc.cgi_bin << "\n";
    }
    if (!loc.nested_locations.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Nested Locations:\n";
        for (size_t k = 0; k < loc.nested_locations.size(); ++k) {
            print_location(loc.nested_locations[k], indent + 4);
        }
    }
}

int main() {
    try {
        std::vector<ServerConfig> configs = parseConfig("file.conf");
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << "Server " << i + 1 << ":\n";
            std::cout << "  Host: " << configs[i].host << "\n";
            std::cout << "  Port: " << configs[i].port << "\n";
            std::cout << "  SSL: " << (configs[i].ssl ? "enabled" : "disabled") << "\n";
            std::cout << "  Server Names: ";
            for (size_t j = 0; j < configs[i].server_names.size(); ++j) {
                std::cout << configs[i].server_names[j] << " ";
            }
            std::cout << "\n";
            std::cout << "  Client Max Body Size: " << configs[i].client_max_body_size << "\n";
            std::cout << "  Root: " << configs[i].root << "\n";
            std::cout << "  Index: ";
            for (size_t j = 0; j < configs[i].index.size(); ++j) {
                std::cout << configs[i].index[j] << " ";
            }
            std::cout << "\n";
            if (!configs[i].ssl_certificate.empty()) {
                std::cout << "  SSL Certificate: " << configs[i].ssl_certificate << "\n";
            }
            if (!configs[i].ssl_certificate_key.empty()) {
                std::cout << "  SSL Certificate Key: " << configs[i].ssl_certificate_key << "\n";
            }
            std::cout << "  Error Pages:\n";
            for (std::map<int, std::string>::iterator it = configs[i].error_pages.begin();
                 it != configs[i].error_pages.end(); ++it) {
                std::cout << "    " << it->first << " -> " << it->second << "\n";
            }
            std::cout << "  Locations:\n";
            for (size_t j = 0; j < configs[i].locations.size(); ++j) {
                print_location(configs[i].locations[j]);
            }
            std::cout << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}