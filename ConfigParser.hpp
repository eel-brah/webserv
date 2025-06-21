/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/14 18:33:49 by muel-bak          #+#    #+#             */
/*   Updated: 2025/06/21 16:09:12 by muel-bak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONFIG_PARSER_HPP
#define CONFIG_PARSER_HPP

#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <stdexcept>

struct LocationConfig {
    std::string path; // e.g., "/api", "= /"
    std::vector<std::string> allowed_methods; // e.g., {"POST", "GET"}
    std::string root; // e.g., "/var/www/api"
    std::string alias; // e.g., "/var/www/static"
    std::vector<std::string> try_files; // e.g., {"$uri", "$uri/", "/index.html"}
    std::string proxy_pass; // e.g., "http://backend:8080"
    std::map<std::string, std::string> proxy_set_headers; // e.g., {"Host", "$host"}
    std::string expires; // e.g., "1y"
    std::string access_log; // e.g., "off"
    std::string auth_basic; // e.g., "Restricted Area"
    std::string auth_basic_user_file; // e.g., "/etc/nginx/.htpasswd"
    std::string deny; // e.g., "all"
    std::string cgi_ext; // e.g., ".php"
    std::string cgi_bin; // e.g., "/usr/bin/php-cgi"
    std::vector<LocationConfig> nested_locations; // For nested location blocks
    std::vector<std::string> index; // e.g., {"index.html", "index.htm"}
    int redirect_code; // e.g., 301
    std::string redirect_url; // e.g., "/newpath"
    bool autoindex; // e.g., true for "on"
    std::string upload_store; // e.g., "/tmp/uploads"
};

struct ServerConfig {
    std::string host; // e.g., "0.0.0.0"
    int port; // e.g., 80
    std::vector<std::string> server_names; // e.g., {"example.com", "www.example.com"}
    size_t client_max_body_size; // e.g., 1048576 (1MB)
    std::map<int, std::string> error_pages; // e.g., {404, "/404.html"}
    std::string root; // e.g., "/var/www/html"
    std::vector<std::string> index; // e.g., {"index.html", "index.htm"}
    std::vector<LocationConfig> locations;
    bool autoindex; // e.g., true for "on"
};

std::vector<ServerConfig> parseConfig(const std::string& file);
bool isPathCompatible(const std::string& locationPath, const std::string& requestedPath);
std::vector<ServerConfig> generateDefaultConfig();

#endif // CONFIG_PARSER_HPP