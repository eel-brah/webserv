/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.cpp                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#             */
/*   Updated: 2025/06/19 14:20:33 by muel-bak         ###   ########.fr       */
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
    if (!loc.redirect.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Redirect: " << loc.redirect << "\n";
    }
    for (int i = 0; i < indent; ++i) std::cout << " ";
    std::cout << "  Autoindex: " << (loc.autoindex ? "on" : "off") << "\n";
    if (!loc.upload_path.empty()) {
        for (int i = 0; i < indent; ++i) std::cout << " ";
        std::cout << "  Upload Path: " << loc.upload_path << "\n";
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

void testPathMatching(const std::vector<ServerConfig>& configs) {
    std::vector<std::string> testPaths;
    testPaths.push_back("/api/users");
    testPaths.push_back("/static/file.css");
    testPaths.push_back("/php/script.php");
    testPaths.push_back("/uploads/file.txt");
    testPaths.push_back("/");
    testPaths.push_back("/index.html");

    for (size_t i = 0; i < configs.size(); ++i) {
        std::cout << "Testing paths for Server " << i + 1 << ":\n";
        for (size_t j = 0; j < configs[i].locations.size(); ++j) {
            const LocationConfig& loc = configs[i].locations[j];
            std::cout << "  Location Path: " << loc.path << "\n";
            for (size_t k = 0; k < testPaths.size(); ++k) {
                bool compatible = isPathCompatible(loc.path, testPaths[k]);
                std::cout << "    Requested Path: " << testPaths[k] << " -> Compatible: " << (compatible ? "true" : "false") << "\n";
            }
            for (size_t k = 0; k < loc.nested_locations.size(); ++k) {
                const LocationConfig& nestedLoc = loc.nested_locations[k];
                std::cout << "    Nested Location Path: " << nestedLoc.path << "\n";
                for (size_t m = 0; m < testPaths.size(); ++m) {
                    bool compatible = isPathCompatible(nestedLoc.path, testPaths[m]);
                    std::cout << "      Requested Path: " << testPaths[m] << " -> Compatible: " << (compatible ? "true" : "false") << "\n";
                }
            }
        }
        std::cout << "\n";
    }
}

int main() {
    try {
        std::vector<ServerConfig> configs = parseConfig("file.conf");
        for (size_t i = 0; i < configs.size(); ++i) {
            std::cout << "Server " << i + 1 << ":\n";
            std::cout << "  Host: " << configs[i].host << "\n";
            std::cout << "  Port: " << configs[i].port << "\n";
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
        testPathMatching(configs);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}