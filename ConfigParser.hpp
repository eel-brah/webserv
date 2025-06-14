/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/14 18:33:49 by muel-bak          #+#    #+#             */
/*   Updated: 2025/06/14 18:33:59 by muel-bak         ###   ########.fr       */
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
    std::string path; // e.g., "/api", "~ \.php$"
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
};

struct ServerConfig {
    std::string host; // e.g., "0.0.0.0"
    int port; // e.g., 80
    bool ssl; // e.g., true for "listen 443 ssl"
    std::vector<std::string> server_names; // e.g., {"example.com", "www.example.com"}
    size_t client_max_body_size; // e.g., 1048576 (1MB)
    std::map<int, std::string> error_pages; // e.g., {404, "/404.html"}
    std::string root; // e.g., "/var/www/html"
    std::vector<std::string> index; // e.g., {"index.html", "index.htm"}
    std::string ssl_certificate; // e.g., "/etc/nginx/ssl/cert.pem"
    std::string ssl_certificate_key; // e.g., "/etc/nginx/ssl/key.pem"
    std::vector<LocationConfig> locations;
};

std::vector<ServerConfig> parseConfig(const std::string& file);

#endif // CONFIG_PARSER_HPP