/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.1337.ma>        +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 15:29:56 by muel-bak          #+#    #+#             */
/*   Updated: 2025/08/09 11:36:25 by muel-bak         ###   ########.fr       */
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

std::string remove_path_info(std::string str, const std::string &substr) {
  std::string::size_type pos = str.rfind(substr);
  if (pos != std::string::npos) {
    str.erase(pos, substr.length());
  }
  return str;
}

int executeCGI(const ServerConfig &server_conf, const std::string &script_path,
               const LocationConfig *location, Client *client) {
  HttpRequest *request = client->get_request();
  if (!request)
    return 500;
  std::string coded_request_path = request->get_path().get_coded_path();

  std::string script_name;
  std::string path_info;
  std::string cgi_bin;
  std::string correct_script_path;
  bool ext_matched = false;

  std::vector<std::string> segments = split(coded_request_path, '/');
  for (size_t i = 1; i <= segments.size(); ++i) {
    std::string candidate = "/";
    for (size_t j = 0; j < i; ++j) {
      candidate += segments[j];
      if (j + 1 < i)
        candidate += "/";
    }

    for (std::map<std::string, std::string>::const_iterator ext_it =
             location->cgi_ext.begin();
         ext_it != location->cgi_ext.end(); ++ext_it) {
      const std::string &ext = ext_it->first;
      if (candidate.size() >= ext.size() &&
          candidate.compare(candidate.size() - ext.size(), ext.size(), ext) ==
              0) {

        script_name = candidate;
        size_t candidate_len = candidate.size();
        if (candidate_len < coded_request_path.size()) {
          path_info = coded_request_path.substr(candidate_len);
        } else {
          path_info.clear();
        }

        correct_script_path =
            remove_path_info(script_path, decode_url(path_info));
        if (!is_valid_path(correct_script_path)) {
          continue;
        }

        cgi_bin = ext_it->second;
        ext_matched = true;
        break;
      }
    }
    if (ext_matched)
      break;
  }

  if (!ext_matched || cgi_bin.empty() ||
      (!is_valid_path(correct_script_path))) {
    LOG_STREAM(WARNING, "CGI: Invalid script path");
    return 404;
  }
  if (!is_valid_path(cgi_bin)) {
    LOG_STREAM(WARNING, "CGI: Invalid configuration or script");
    return 403;
  }

  int input_pipe[2] = {-1, -1};  // Parent writes to child stdin
  int output_pipe[2] = {-1, -1}; // Child writes to parent stdout
  if (pipe(input_pipe) == -1 || pipe(output_pipe) == -1) {
    LOG_STREAM(ERROR, "CGI: Pipe creation failed: " << strerror(errno));
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    return 503; // Service Unavailable (resource allocation failure)
  }

  // Set output pipe to non-blocking
  if (fcntl(output_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
    LOG_STREAM(ERROR, "CGI: fcntl failed: " << strerror(errno));
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    return 503; // Service Unavailable (resource allocation failure)
  }

  cgi_child_pid = fork();
  if (cgi_child_pid == -1) {
    LOG_STREAM(ERROR, "CGI: Fork failed: " << strerror(errno));
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    return 503; // Service Unavailable (resource allocation failure)
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

    // Redirect stderr to a file for logging
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

    env_stream << "SCRIPT_NAME=" << script_name;
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

    env_stream << "SERVER_PORT=" << client->port;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "SERVER_PROTOCOL=HTTP/1.1";
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "SERVER_SOFTWARE=" << SERVER_SOFTWARE;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "QUERY_STRING=" << request->get_path().get_coded_queries();
    ;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "REMOTE_ADDR=" << client->addr;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "REMOTE_HOST=" << client->addr;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    try {
      std::string cookie = request->get_header_by_key("cookie")->value;
      env_stream << "HTTP_COOKIE=" << cookie;
      env_strings.push_back(env_stream.str());
      env_stream.str("");

    } catch (std::exception &e) {
    }

    // TODO: chunk case
    size_t content_length = request->get_body_len();
    if (content_length != 0) {
      env_stream << "CONTENT_LENGTH=" << content_length;
      env_strings.push_back(env_stream.str());
      env_stream.str("");
    }

    try {
      std::string content_type = "";
      content_type = request->get_header_by_key("content-type")->value;
      env_stream << "CONTENT_TYPE=" << content_type;
      env_strings.push_back(env_stream.str());
      env_stream.str("");
    } catch (std::exception &e) {
    }

    env_stream << "PATH_INFO=" << path_info;
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    if (!path_info.empty()) {
      std::string path_translated;

      path_translated = join_paths(location->root, decode_url(path_info));

      env_stream << "PATH_TRANSLATED=" << path_translated;
      env_strings.push_back(env_stream.str());
      env_stream.str("");
    }

    for (size_t i = 0; i < env_strings.size(); ++i) {
      envp.push_back(const_cast<char *>(env_strings[i].c_str()));
    }
    envp.push_back(0);

    std::vector<std::string> argv_strings;
    std::vector<char *> argv;
    argv_strings.push_back(cgi_bin);
    argv_strings.push_back(correct_script_path);
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
      return 503; // Service Unavailable (file system issue)
    }
    char buffer[4096] = {0};
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
          return 500; 
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
    return 503; // Service Unavailable (file system issue)
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
      return 503; // Service Unavailable (resource issue)
    } else if (select_result == 0) {
      // Timeout occurred
      LOG_STREAM(ERROR, "CGI: Timeout after " << timeout_secs << " seconds");
      kill(cgi_child_pid, SIGTERM);
      close(output_fd);
      close(output_pipe[0]);
      unlink(temp_output_file.c_str());
      return 504; // Gateway Timeout
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
          return 503; // Service Unavailable (file system issue)
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
        return 500;
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
        return 502; // Bad Gateway (CGI script failure)
      }
      break;
    } else if (wait_result == -1) {
      LOG_STREAM(ERROR, "CGI: waitpid failed: " << strerror(errno));
      close(output_fd);
      close(output_pipe[0]);
      unlink(temp_output_file.c_str());
      return 503; // Service Unavailable (resource issue)
    }
  }

  close(output_fd);
  close(output_pipe[0]);

  if (!data_received) {
    LOG_STREAM(ERROR, "CGI: No data received from child process");
    unlink(temp_output_file.c_str());
    return 502; // Bad Gateway (no output from CGI)
  }

  std::ifstream cgi_output_file(temp_output_file.c_str(), std::ios::binary);
  if (!cgi_output_file) {
    LOG_STREAM(ERROR, "CGI: Failed to read temp file");
    unlink(temp_output_file.c_str());
    return 503; // Service Unavailable (file system issue)
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
    return 502; 
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
      cgi_headers.find("content-length:") != std::string::npos ||
      cgi_headers.find("Content-Length:") != std::string::npos;

  response_stream << "HTTP/1.1 " << http_status << "\r\n"
                  << get_server_header() + get_date_header() << cgi_headers
                  << "\r\n";
  if (!has_content_length) {
    response_stream << "content-length: " << cgi_body.size() << "\r\n";
  }
  response_stream << "\r\n" << cgi_body;

  client->response = response_stream.str();
  return 0;
}
