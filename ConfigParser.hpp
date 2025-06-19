/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ConfigParser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 17:17:38 by muel-bak          #+#    #+#             */
/*   Updated: 2025/06/19 14:13:30 by muel-bak         ###   ########.fr       */
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
    std::string path; // e.g., "/api"
    std::vector<std::string> allowed_methods; // e.g., {"GET", "POST"}
    std::string root; // e.g., "/var/www/api"
    std::string alias; // e.g., "/var/www/static"
    std::vector<std::string> try_files; // e.g., {"$uri", "$uri/", "/index.html"}
    std::string redirect; // e.g., "http://example.com/newpath"
    bool autoindex; // Directory listing: true = on, false = off
    std::vector<std::string> index; // e.g., {"index.html", "index.htm"}
    std::string cgi_ext; // e.g., ".php"
    std::string cgi_bin; // e.g., "/usr/bin/php-cgi"
    std::string upload_path; // e.g., "/var/www/uploads"
    std::vector<LocationConfig> nested_locations; // For nested location blocks
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
};

std::vector<ServerConfig> parseConfig(const std::string& file);
bool isPathCompatible(const std::string& locationPath, const std::string& requestedPath);
bool endsWithAnyCaseInsensitive(const std::string& str, const std::string& suffixes);

#endif // CONFIG_PARSER_HPP