/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 15:29:56 by muel-bak          #+#    #+#             */
/*   Updated: 2025/08/02 16:41:14 by muel-bak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ConfigParser.hpp"
#include "../include/parser.hpp"
#include "../include/webserv.hpp"
#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <string.h>
#include <sys/select.h> // Added for select
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

static pid_t cgi_child_pid = -1;

void handle_cgi_timeout(int sig) {
  (void)sig;
  if (cgi_child_pid > 0) {
    kill(cgi_child_pid, SIGTERM);
    cgi_child_pid = -1;
  }
}

bool is_valid_path(const std::string &path) {
  // std::cerr << "Checking path: " << path << std::endl;
  if (path.empty()) {
    std::cerr << "Path invalid: empty" << std::endl;
    return false;
  }
  if (path.find("..") != std::string::npos) {
    std::cerr << "Path invalid: contains '..'" << std::endl;
    return false;
  }
  if (path[0] != '/' &&
      !(path.length() >= 2 && path[0] == '.' && path[1] == '/')) {
    std::cerr << "Path invalid: does not start with '/' or './'" << std::endl;
    return false;
  }
  struct stat file_stat;
  if (stat(path.c_str(), &file_stat) != 0) {
    std::cerr << "Path invalid: stat failed with error: " << strerror(errno)
              << std::endl;
    return false;
  }
  if (!S_ISREG(file_stat.st_mode)) {
    std::cerr << "Path invalid: not a regular file" << std::endl;
    return false;
  }
  // std::cerr << "Path valid: regular file" << std::endl;
  return true;
}

// void debugCGI(const ServerConfig &server_conf, const std::string
// &script_path,
//               const std::string &request_path,
//               const LocationConfig *matched_location, bool ext_matched,
//               const std::string &cgi_bin) {
//   std::cerr << "=== CGI Debug Start ===" << std::endl;
//
//   std::cerr << "ServerConfig Details:" << std::endl;
//   std::cerr << "  Port: " << server_conf.getPort() << std::endl;
//   std::cerr << "  Root: " << server_conf.getRoot() << std::endl;
//   std::cerr << "  Server Names: ";
//   const std::vector<std::string> &server_names =
//   server_conf.getServerNames(); for (size_t i = 0; i < server_names.size();
//   ++i) {
//     std::cerr << server_names[i] << (i < server_names.size() - 1 ? ", " :
//     "");
//   }
//   std::cerr << std::endl;
//
//   std::cerr << "Location Matching:" << std::endl;
//   std::cerr << "  Request Path: " << request_path << std::endl;
//   if (matched_location) {
//     std::cerr << "  Matched Location Path: " << matched_location->path
//               << std::endl;
//     std::cerr << "  Location Root: " << matched_location->root << std::endl;
//     std::cerr << "  CGI Extensions: ";
//     if (matched_location->cgi_ext.empty()) {
//       std::cerr << "None" << std::endl;
//     } else {
//       for (std::map<std::string, std::string>::const_iterator it =
//                matched_location->cgi_ext.begin();
//            it != matched_location->cgi_ext.end(); ++it) {
//         std::cerr << it->first << " -> " << it->second << " ";
//       }
//       std::cerr << std::endl;
//     }
//   } else {
//     std::cerr << "  No matched location found for request_path: "
//               << request_path << std::endl;
//     std::cerr << "  Available Locations: ";
//     const std::vector<LocationConfig> &locations =
//     server_conf.getLocations(); for
//     (std::vector<LocationConfig>::const_iterator it = locations.begin();
//          it != locations.end(); ++it) {
//       std::cerr << it->path << " ";
//     }
//     std::cerr << std::endl;
//   }
//
//   std::cerr << "Extension Matching:" << std::endl;
//   std::cerr << "  Script Path: " << script_path << std::endl;
//   std::cerr << "  Extension Matched: " << (ext_matched ? "Yes" : "No")
//             << std::endl;
//   std::cerr << "  CGI Binary: " << (cgi_bin.empty() ? "Empty" : cgi_bin)
//             << std::endl;
//
//   std::cerr << "Path Validation:" << std::endl;
//   std::cerr << "  Script Path (" << script_path << "):" << std::endl;
//   if (script_path.empty()) {
//     std::cerr << "    Empty path" << std::endl;
//   } else if (script_path.find("..") != std::string::npos) {
//     std::cerr << "    Contains '..' (directory traversal)" << std::endl;
//   } else if (script_path[0] != '/' &&
//              !(script_path.length() >= 2 && script_path[0] == '.' &&
//                script_path[1] == '/')) {
//     std::cerr << "    Does not start with '/' or './'" << std::endl;
//   } else {
//     struct stat file_stat;
//     if (stat(script_path.c_str(), &file_stat) != 0) {
//       std::cerr << "    Stat failed: " << strerror(errno) << std::endl;
//     } else if (!S_ISREG(file_stat.st_mode)) {
//       std::cerr << "    Not a regular file" << std::endl;
//     } else {
//       std::cerr << "    Valid regular file" << std::endl;
//     }
//   }
//
//   std::cerr << "  CGI Binary Path (" << cgi_bin << "):" << std::endl;
//   if (cgi_bin.empty()) {
//     std::cerr << "    Empty CGI binary path" << std::endl;
//   } else if (cgi_bin.find("..") != std::string::npos) {
//     std::cerr << "    Contains '..' (directory traversal)" << std::endl;
//   } else if (cgi_bin[0] != '/') {
//     std::cerr << "    Does not start with '/'" << std::endl;
//   } else {
//     struct stat file_stat;
//     if (stat(cgi_bin.c_str(), &file_stat) != 0) {
//       std::cerr << "    Stat failed: " << strerror(errno) << std::endl;
//     } else if (!S_ISREG(file_stat.st_mode)) {
//       std::cerr << "    Not a regular file" << std::endl;
//     } else {
//       std::cerr << "    Valid regular file" << std::endl;
//     }
//   }
//
//   std::cerr << "=== CGI Debug End ===" << std::endl;
// }

bool Client::executeCGI(const ServerConfig &server_conf,
                        const std::string &script_path,
                        const LocationConfig *location) {
  std::string request_path = request->get_path().get_path();

  // Extract query string from request_path
  // TODO: The full request and arguments provided by the client must be
  // available to the CGI.
  std::string query_string = request->get_path().get_queries();

  // Validate CGI configuration and script path
  std::string cgi_bin;
  bool ext_matched = false;
  for (std::map<std::string, std::string>::const_iterator ext_it =
           location->cgi_ext.begin();
       ext_it != location->cgi_ext.end(); ++ext_it) {
    if (endsWith(script_path, ext_it->first)) {
      cgi_bin = ext_it->second;
      ext_matched = true;
      break;
    }
  }

  if (!ext_matched || cgi_bin.empty() || !is_valid_path(script_path) ||
      !is_valid_path(cgi_bin)) {
    LOG_STREAM(ERROR, "CGI: Invalid configuration or script path");
    return false;
  }

  int input_pipe[2] = {-1, -1};  // Parent writes to child stdin
  int output_pipe[2] = {-1, -1}; // Child writes to parent stdout
  if (pipe(input_pipe) == -1 || pipe(output_pipe) == -1) {
    LOG_STREAM(ERROR, "CGI: Pipe creation failed: " << strerror(errno));
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    return false;
  }

  // Set output pipe to non-blocking
  if (fcntl(output_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
    LOG_STREAM(ERROR, "CGI: fcntl failed: " << strerror(errno));
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    return false;
  }

  cgi_child_pid = fork();
  if (cgi_child_pid == -1) {
    LOG_STREAM(ERROR, "CGI: Fork failed: " << strerror(errno));
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    return false;
  }

  if (cgi_child_pid == 0) {
    // Child process
    close(input_pipe[1]);
    close(output_pipe[0]);

    // Redirect stdin and stdout
    if (dup2(input_pipe[0], STDIN_FILENO) == -1 ||
        dup2(output_pipe[1], STDOUT_FILENO) == -1) {
      LOG_STREAM(ERROR, "CGI: dup2 failed: " << strerror(errno));
      exit(1);
    }
    close(input_pipe[0]);
    close(output_pipe[1]);

    // Redirect stderr to a file for debugging
    std::stringstream pid_ss;
    pid_ss << getpid();
    std::string stderr_file = "/tmp/cgi_stderr_" + pid_ss.str();
    int stderr_fd =
        open(stderr_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
    if (stderr_fd == -1) {
      LOG_STREAM(ERROR, "CGI: Failed to open stderr file: " << strerror(errno));
      exit(1);
    }
    if (dup2(stderr_fd, STDERR_FILENO) == -1) {
      LOG_STREAM(ERROR, "CGI: dup2 stderr failed: " << strerror(errno));
      close(stderr_fd);
      exit(1);
    }
    close(stderr_fd);

    // Set up CGI environment variables
    std::vector<std::string> env_strings;
    std::vector<char *> envp;
    std::stringstream env_stream;

    std::string method_str = "GET";
    if (request) {
      switch (request->get_method()) {
      case GET:
        method_str = "GET";
        break;
      case POST:
        method_str = "POST";
        break;
      case DELETE:
        method_str = "DELETE";
        break;
      default:
        method_str = "GET";
      }
    }

    env_stream << "GATEWAY_INTERFACE=" << "CGI/0.1";
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "REQUEST_METHOD=" << method_str;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "SCRIPT_NAME=" << script_path;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    const std::vector<std::string> &server_names = server_conf.getServerNames();
    env_stream << "SERVER_NAME="
               << (server_names.empty() ? "localhost" : server_names[0]);
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    try {
      env_stream << "SERVER_NAME=" << request->get_header_by_key("host")->value;
      env_strings.push_back(env_stream.str());
      env_stream.str("");
    } catch (std::exception &e) {
    }

    env_stream << "SERVER_PORT=" << port;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "SERVER_PROTOCOL=HTTP/1.1";
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "SERVER_SOFTWARE=" << SERVER_SOFTWARE;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    // TODO: fix this
    env_stream << "QUERY_STRING=" << query_string;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "REMOTE_ADDR=" << addr;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    try {
      std::string content_length = "0";
      content_length = request->get_header_by_key("content-length")->value;
      env_stream << "CONTENT_LENGTH=" << content_length;
      env_strings.push_back(env_stream.str());
      env_stream.str("");
    } catch (std::exception &e) {
    }

    try {
      std::string content_type = "";
      content_type = request->get_header_by_key("content-type")->value;
      env_stream << "CONTENT_TYPE=" << content_type;
      env_strings.push_back(env_stream.str());
      env_stream.str("");
    } catch (std::exception &e) {
    }

    // TODO:
    // env_stream << "PATH_INFO=" << request_path;
    // env_strings.push_back(env_stream.str());
    // env_stream.str("");
    // PATH_TRANSLATED

    for (size_t i = 0; i < env_strings.size(); ++i) {
      envp.push_back(const_cast<char *>(env_strings[i].c_str()));
    }
    envp.push_back(0);

    std::vector<std::string> argv_strings;
    std::vector<char *> argv;
    argv_strings.push_back(cgi_bin);
    argv_strings.push_back(script_path);
    for (size_t i = 0; i < argv_strings.size(); ++i) {
      argv.push_back(const_cast<char *>(argv_strings[i].c_str()));
    }
    argv.push_back(0);

    execve(cgi_bin.c_str(), argv.data(), envp.data());
    LOG_STREAM(ERROR, "CGI: execve failed: " << strerror(errno));
    exit(1);
  }

  close(input_pipe[0]);
  close(output_pipe[1]);

  // std::string body = request->body;
  // if (!body.empty()) {
  //   size_t written = 0;
  //   while (written < body.size()) {
  //     ssize_t ret =
  //         write(input_pipe[1], body.c_str() + written, body.size() -
  //         written);
  //     if (ret == -1) {
  //       LOG_STREAM(ERROR,
  //                  "CGI: Write to input pipe failed: " << strerror(errno));
  //       kill(cgi_child_pid, SIGTERM);
  //       close(input_pipe[1]);
  //       close(output_pipe[0]);
  //       return false;
  //     }
  //     written += ret;
  //   }
  // }

  // if 'body' contains the file path
  std::string body_path = request->body;
  if (!body_path.empty()) {
    std::ifstream body_file(body_path.c_str(), std::ios::binary);
    if (!body_file.is_open()) {
      LOG_STREAM(ERROR, "CGI: Failed to open body file: " << strerror(errno));
      kill(cgi_child_pid, SIGTERM);
      close(input_pipe[1]);
      close(output_pipe[0]);
      return false;
    }
    char buffer[4096];
    size_t written = 0;
    while (body_file) {
      body_file.read(buffer, sizeof(buffer));
      std::streamsize bytes_read = body_file.gcount();

      if (bytes_read > 0) {
        ssize_t ret =
            write(input_pipe[1], buffer, static_cast<size_t>(bytes_read));
        if (ret == -1) {
          LOG_STREAM(ERROR,
                     "CGI: Write to input pipe failed: " << strerror(errno));
          kill(cgi_child_pid, SIGTERM);
          close(input_pipe[1]);
          close(output_pipe[0]);
          return false;
        }
        written += ret;
      }
    }
    body_file.close();
  }

  close(input_pipe[1]);

  std::stringstream pid_ss;
  pid_ss << cgi_child_pid;
  std::string temp_output_file = "/tmp/cgi_output_" + pid_ss.str();
  int output_fd =
      open(temp_output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (output_fd == -1) {
    LOG_STREAM(ERROR, "CGI: Failed to open temp file: " << strerror(errno));
    kill(cgi_child_pid, SIGTERM);
    close(output_pipe[0]);
    return false;
  }

  // Timeout handling using select
  const int timeout_secs = 10;
  char buffer[1024];
  ssize_t bytes_read;
  bool data_received = false;
  struct timeval tv;
  fd_set read_fds;

  while (true) {
    FD_ZERO(&read_fds);
    FD_SET(output_pipe[0], &read_fds);
    tv.tv_sec = timeout_secs;
    tv.tv_usec = 0;

    int select_result = select(output_pipe[0] + 1, &read_fds, 0, 0, &tv);
    if (select_result == -1) {
      LOG_STREAM(ERROR, "CGI: select failed: " << strerror(errno));
      kill(cgi_child_pid, SIGTERM);
      close(output_fd);
      close(output_pipe[0]);
      unlink(temp_output_file.c_str());
      return false;
    } else if (select_result == 0) {
      // Timeout occurred
      LOG_STREAM(ERROR, "CGI: Timeout after " << timeout_secs << " seconds");
      kill(cgi_child_pid, SIGTERM);
      close(output_fd);
      close(output_pipe[0]);
      unlink(temp_output_file.c_str());
      return false;
    }

    if (FD_ISSET(output_pipe[0], &read_fds)) {
      bytes_read = read(output_pipe[0], buffer, sizeof(buffer));
      if (bytes_read > 0) {
        data_received = true;
        if (write(output_fd, buffer, bytes_read) == -1) {
          LOG_STREAM(ERROR,
                     "CGI: Write to temp file failed: " << strerror(errno));
          close(output_fd);
          close(output_pipe[0]);
          kill(cgi_child_pid, SIGTERM);
          unlink(temp_output_file.c_str());
          return false;
        }
      } else if (bytes_read == 0) {
        // EOF reached
        break;
      } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
        LOG_STREAM(ERROR,
                   "CGI: Read from output pipe failed: " << strerror(errno));
        close(output_fd);
        close(output_pipe[0]);
        kill(cgi_child_pid, SIGTERM);
        unlink(temp_output_file.c_str());
        return false;
      }
    }

    // Check if child process has exited
    int wait_status;
    pid_t wait_result = waitpid(cgi_child_pid, &wait_status, WNOHANG);
    if (wait_result == cgi_child_pid) {
      cgi_child_pid = -1;
      if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0) {
        LOG_STREAM(ERROR, "CGI: Child process failed: "
                              << (WIFEXITED(wait_status)
                                      ? int_to_string(WEXITSTATUS(wait_status))
                                      : "abnormal termination"));
        close(output_fd);
        close(output_pipe[0]);
        unlink(temp_output_file.c_str());
        return false;
      }
      break;
    } else if (wait_result == -1) {
      LOG_STREAM(ERROR, "CGI: waitpid failed: " << strerror(errno));
      close(output_fd);
      close(output_pipe[0]);
      unlink(temp_output_file.c_str());
      return false;
    }
  }

  close(output_fd);
  close(output_pipe[0]);

  if (!data_received) {
    LOG_STREAM(ERROR, "CGI: No data received from child process");
    unlink(temp_output_file.c_str());
    return false;
  }

  std::ifstream cgi_output_file(temp_output_file.c_str(), std::ios::binary);
  if (!cgi_output_file) {
    LOG_STREAM(ERROR, "CGI: Failed to read temp file");
    unlink(temp_output_file.c_str());
    return false;
  }
  std::stringstream cgi_output;
  cgi_output << cgi_output_file.rdbuf();
  cgi_output_file.close();
  if (unlink(temp_output_file.c_str()) == -1) {
    LOG_STREAM(ERROR, "Failed to delete " << temp_output_file << ": "
                                          << strerror(errno));
  }

  std::string cgi_content = cgi_output.str();
  size_t header_end = cgi_content.find("\r\n\r\n");
  if (header_end == std::string::npos) {
    LOG_STREAM(ERROR, "CGI: Invalid output format");
    LOG_STREAM(DEBUG, cgi_content);
    return false;
  }

  std::string cgi_headers = cgi_content.substr(0, header_end);
  std::string cgi_body = cgi_content.substr(header_end + 4);
  std::string http_status = "200 OK";

  std::istringstream header_stream(cgi_headers);
  std::string line;
  while (std::getline(header_stream, line) && !line.empty() && line != "\r") {
    if (line.find("Status:") == 0) {
      http_status = line.substr(7);
      size_t cr_pos = http_status.find('\r');
      if (cr_pos != std::string::npos)
        http_status = http_status.substr(0, cr_pos);
      http_status = http_status.substr(http_status.find_first_not_of(" \t"));
    }
  }

  std::stringstream response_stream;

  bool has_content_length =
      cgi_headers.find("content-length:") != std::string::npos;

  response_stream << "HTTP/1.1 " << http_status << "\r\n"
                  << get_server_header() + get_date_header() << cgi_headers
                  << "\r\n";
  if (!has_content_length) {
    response_stream << "content-length: " << cgi_body.size() << "\r\n";
  }
  response_stream << "\r\n" << cgi_body;

  response = response_stream.str();
  return true;
}
