/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   cgi.cpp                                            :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: muel-bak <muel-bak@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/08 15:29:56 by muel-bak          #+#    #+#             */
/*   Updated: 2025/08/10 15:14:07 by muel-bak         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../include/ConfigParser.hpp"
#include "../include/parser.hpp"
#include "../include/webserv.hpp"

static pid_t cgi_child_pid = -1;
std::map<int, Client *> cgi_to_client;

void wait_for_child() {
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
}

void cgi_cleanup(int epoll_fd, Client *client) {
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.pipe_fd, NULL);
  epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.in_pipe_fd, NULL);
  close(client->cgi.output_fd);
  close(client->cgi.pipe_fd);
  close(client->cgi.in_pipe_fd);
  close(client->cgi.body_fd);
  remove(client->cgi.output_file.c_str());
  cgi_to_client.erase(client->cgi.pipe_fd);
  cgi_to_client.erase(client->cgi.in_pipe_fd);
}

std::string get_script_dir(std::string path) {
  if (path.size() > 1) {
    size_t pos = path.rfind('/');
    if (pos != std::string::npos) {
      path.erase(pos + 1);
    }
  }
  return path;
}

std::string get_script_name(std::string path) {
  size_t pos = path.rfind('/');
  std::string result;
  if (pos != std::string::npos) {
    result = path.substr(pos);
  } else {
    result = path;
  }
  return result;
}

std::string header_trim(const std::string &str) {
  std::string::size_type start = str.find_first_not_of(" \t\r");
  if (start == std::string::npos)
    return "";
  std::string::size_type end = str.find_last_not_of(" \t\r");
  return str.substr(start, end - start + 1);
}

bool is_valid_path(const std::string &path) {
  if (path.empty()) {
    LOG_STREAM(ERROR, "Path invalid: empty");
    return false;
  }
  if (path.find("..") != std::string::npos) {
    LOG_STREAM(ERROR, "Path invalid: contains '..'");
    return false;
  }
  if (path[0] != '/' &&
      !(path.length() >= 2 && path[0] == '.' && path[1] == '/')) {
    LOG_STREAM(ERROR, "Path invalid: does not start with '/' or './'");
    return false;
  }
  struct stat file_stat;
  if (stat(path.c_str(), &file_stat) != 0) {
    LOG_STREAM(ERROR,
               "Path invalid: stat failed with error: " << strerror(errno));
    return false;
  }
  if (!S_ISREG(file_stat.st_mode)) {
    LOG_STREAM(ERROR, "Path invalid: not a regular file");
    return false;
  }
  return true;
}

std::string remove_path_info(std::string str, const std::string &substr) {
  std::string::size_type pos = str.rfind(substr);
  if (pos != std::string::npos) {
    str.erase(pos, substr.length());
  }
  return str;
}

int executeCGI(int epoll_fd, const ServerConfig &server_conf,
               const std::string &script_path, const LocationConfig *location,
               Client *client) {
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
    return 503;
  }

  // Set output pipe to non-blocking
  if (fcntl(output_pipe[0], F_SETFL, O_NONBLOCK) == -1) {
    LOG_STREAM(ERROR, "CGI: fcntl failed: " << strerror(errno));
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    return 503;
  }

  cgi_child_pid = fork();
  if (cgi_child_pid == -1) {
    LOG_STREAM(ERROR, "CGI: Fork failed: " << strerror(errno));
    close(input_pipe[0]);
    close(input_pipe[1]);
    close(output_pipe[0]);
    close(output_pipe[1]);
    return 503;
  }

  if (cgi_child_pid == 0) {

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

    // Set up CGI environment variables
    std::vector<std::string> env_strings;
    std::vector<char *> envp;
    std::stringstream env_stream;

    env_stream << "GATEWAY_INTERFACE=" << "CGI/0.1";
    env_strings.push_back(env_stream.str());
    env_stream.str("");

    env_stream << "REQUEST_METHOD=" << method_to_string(request->get_method());
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
      std::string host = request->get_header_by_key("host")->value;
      env_stream << "SERVER_NAME=" << host;
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

    size_t content_length = request->get_body_len();
    if (content_length != 0) {
      env_stream << "CONTENT_LENGTH=" << content_length;
      env_strings.push_back(env_stream.str());
      env_stream.str("");
    }

    try {
      std::string content_type =
          request->get_header_by_key("content-type")->value;
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

    std::string script_dir = get_script_dir(correct_script_path);
    script_name = "." + get_script_name(correct_script_path);

    std::vector<std::string> argv_strings;
    std::vector<char *> argv;
    argv_strings.push_back(cgi_bin);
    argv_strings.push_back(script_name);
    for (size_t i = 0; i < argv_strings.size(); ++i) {
      argv.push_back(const_cast<char *>(argv_strings[i].c_str()));
    }
    argv.push_back(0);

    if (chdir(script_dir.c_str()) == -1) {
      LOG_STREAM(ERROR, "CGI: chdir failed: " << strerror(errno));
      exit(1);
    }

    execve(cgi_bin.c_str(), argv.data(), envp.data());
    LOG_STREAM(ERROR, "CGI: execve failed: " << strerror(errno));
    exit(1);
  }

  close(input_pipe[0]);
  close(output_pipe[1]);
  client->cgi.pipe_fd = output_pipe[0];
  client->cgi.pid = cgi_child_pid;

  std::string body_path = request->body;
  if (!1) {
    int fd = open(body_path.c_str(), O_RDONLY);
    if (fd < 0) {
      LOG_STREAM(ERROR, "Error opening body file: " << strerror(errno));
      kill(cgi_child_pid, SIGTERM);
      close(input_pipe[1]);
      close(output_pipe[0]);
      return 503;
    }
    client->cgi.body_fd = fd;
    client->cgi.in_pipe_fd = input_pipe[1];

    struct epoll_event ev;
    ev.events = EPOLLOUT;
    ev.data.fd = client->cgi.in_pipe_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->cgi.in_pipe_fd, &ev) == -1) {
      LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
      kill(cgi_child_pid, SIGTERM);
      cgi_cleanup(epoll_fd, client);
      return 500;
    }
    cgi_to_client[client->cgi.in_pipe_fd] = client;
  } else {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = client->cgi.pipe_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->cgi.pipe_fd, &ev) == -1) {
      LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
      kill(cgi_child_pid, SIGTERM);
      cgi_cleanup(epoll_fd, client);
      return 500;
    }
    client->cgi.start = std::time(NULL);
    cgi_to_client[client->cgi.pipe_fd] = client;
  }

  std::string temp_output_file = "/tmp/cgi_" + random_string();
  int output_fd =
      open(temp_output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);
  if (output_fd == -1) {
    LOG_STREAM(ERROR, "CGI: Failed to open temp file: " << strerror(errno));
    kill(client->cgi.pid, SIGTERM);
    cgi_cleanup(epoll_fd, client);
    return 503;
  }
  client->cgi.output_fd = output_fd;
  client->cgi.output_file = temp_output_file;

  return 0;
}

void in_cgi_cleanup(int epoll_fd, Client *client) {
  close(client->cgi.output_fd);
  if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.pipe_fd, NULL) == -1)
    LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
  close(client->cgi.pipe_fd);
  kill(client->cgi.pid, SIGTERM);
  remove(client->cgi.output_file.c_str());
  cgi_to_client.erase(client->cgi.pipe_fd);
}

int prepare_cgi_response(int epoll_fd, Client *client, bool check) {
  if (check) {
    int wait_status;
    pid_t wait_result = waitpid(client->cgi.pid, &wait_status, WNOHANG);
    if (wait_result == client->cgi.pid) {
      client->cgi.pid = -1;
      if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0) {
        LOG_STREAM(ERROR, "CGI: Child process failed: "
                              << (WIFEXITED(wait_status)
                                      ? int_to_string(WEXITSTATUS(wait_status))
                                      : "abnormal termination"));
        cgi_cleanup(epoll_fd, client);
        return 502;
      }
    } else if (wait_result == -1) {
      LOG_STREAM(ERROR, "CGI: waitpid failed: " << strerror(errno));
      cgi_cleanup(epoll_fd, client);
      return 503;
    }
  }

  close(client->cgi.output_fd);
  if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.pipe_fd, NULL) == -1)
    LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
  close(client->cgi.pipe_fd);
  cgi_to_client.erase(client->cgi.pipe_fd);

  if (!client->cgi.data_received) {
    LOG_STREAM(ERROR, "CGI: No data received from child process");
    remove(client->cgi.output_file.c_str());
    return 502;
  }

  std::ifstream cgi_output_file(client->cgi.output_file.c_str(),
                                std::ios::binary);
  if (!cgi_output_file) {
    LOG_STREAM(ERROR, "CGI: Failed to read temp file");
    remove(client->cgi.output_file.c_str());
    return 503;
  }
  std::stringstream cgi_output;
  cgi_output << cgi_output_file.rdbuf();
  cgi_output_file.close();
  remove(client->cgi.output_file.c_str());

  std::string cgi_content = cgi_output.str();
  size_t header_end = cgi_content.find("\r\n\r\n");
  if (header_end == std::string::npos) {
    LOG_STREAM(ERROR, "CGI: Invalid output format");
    return 502;
  }

  std::string cgi_headers = cgi_content.substr(0, header_end);
  std::string cgi_body = cgi_content.substr(header_end + 4);
  std::string http_status = "200 OK";

  size_t provided_content_length = 0;
  bool has_content_length = false;
  bool has_content_type = false;
  std::istringstream header_stream(cgi_headers);
  std::string line;

  while (std::getline(header_stream, line) && !line.empty() && line != "\r") {
    std::string::size_type colon_pos = line.find(':');
    if (colon_pos == std::string::npos)
      continue;

    std::string key = to_lower(line.substr(0, colon_pos));
    std::string value = header_trim(line.substr(colon_pos + 1));

    if (key == "status") {
      http_status = value;
    } else if (key == "content-length") {
      has_content_length = true;
      provided_content_length = ft_atoi(value.c_str());
    } else if (key == "content-type") {
      has_content_type = true;
    }
  }
  if (!has_content_type) {
    return 502;
  }
  std::stringstream response_stream;
  response_stream << "HTTP/1.1 " << http_status << "\r\n"
                  << get_server_header() + get_date_header() << cgi_headers
                  << "\r\n";
  if (!has_content_length || provided_content_length != cgi_body.size()) {
    response_stream << "content-length: " << cgi_body.size() << "\r\n";
  }
  response_stream << "\r\n" << cgi_body;

  client->response = response_stream.str();
  client->clear_cgi();
  return 0;
}

int handle_cgi(int epoll_fd, Client *client, uint32_t actions) {
  char buffer[4096] = {0};
  ssize_t bytes_read;
  ssize_t written;
  ssize_t ret;

  if (actions & EPOLLERR) {
    LOG(ERROR, "epoll: pipe error");
    cgi_cleanup(epoll_fd, client);
    return 500;
  }

  if (actions & EPOLLHUP) {
    int wait_status;
    pid_t wait_result = waitpid(client->cgi.pid, &wait_status, WNOHANG);
    if (wait_result == client->cgi.pid) {
      client->cgi.pid = -1;
      if (!WIFEXITED(wait_status) || WEXITSTATUS(wait_status) != 0) {
        LOG_STREAM(ERROR, "CGI: Child process failed: "
                              << (WIFEXITED(wait_status)
                                      ? int_to_string(WEXITSTATUS(wait_status))
                                      : "abnormal termination"));
        cgi_cleanup(epoll_fd, client);
        return 502;
      }
    } else if (wait_result == -1) {
      LOG_STREAM(ERROR, "CGI: waitpid failed: " << strerror(errno));
      cgi_cleanup(epoll_fd, client);
      return 503;
    }
    return prepare_cgi_response(epoll_fd, client, false);
  }

  if (actions & EPOLLOUT && client->cgi.body_fd != -1) {
    bytes_read = read(client->cgi.body_fd, buffer, sizeof(buffer));
    if (bytes_read > 0) {
      written = 0;
      while (written < bytes_read) {
        ret = write(client->cgi.in_pipe_fd, buffer, bytes_read);
        if (ret < 0) {
          LOG_STREAM(ERROR,
                     "CGI: Write to input pipe failed: " << strerror(errno));

          if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.in_pipe_fd,
                        NULL) == -1)
            LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
          kill(client->cgi.pid, SIGTERM);
          if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.in_pipe_fd,
                        NULL) == -1)
            LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
          close(client->cgi.in_pipe_fd);
          close(client->cgi.pipe_fd);
          close(client->cgi.body_fd);
          close(client->cgi.output_fd);
          remove(client->cgi.output_file.c_str());
          cgi_to_client.erase(client->cgi.in_pipe_fd);
          return 503;
        }
        written += ret;
      }
      return -1;
    } else if (bytes_read < 0) {
      LOG_STREAM(ERROR, "CGI: Read from body file failed: " << strerror(errno));
      if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.in_pipe_fd, NULL) ==
          -1)
        LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
      kill(client->cgi.pid, SIGTERM);
      if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.in_pipe_fd, NULL) ==
          -1)
        LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
      close(client->cgi.in_pipe_fd);
      close(client->cgi.pipe_fd);
      close(client->cgi.body_fd);
      close(client->cgi.output_fd);
      remove(client->cgi.output_file.c_str());
      cgi_to_client.erase(client->cgi.in_pipe_fd);
      return 500;
    } else {
      if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->cgi.in_pipe_fd, NULL) ==
          -1)
        LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
      close(client->cgi.in_pipe_fd);
      close(client->cgi.body_fd);
      client->cgi.in_pipe_fd = -1;
      client->cgi.body_fd = -1;
      cgi_to_client.erase(client->cgi.in_pipe_fd);

      struct epoll_event ev;
      ev.events = EPOLLIN;
      ev.data.fd = client->cgi.pipe_fd;
      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client->cgi.pipe_fd, &ev) == -1) {
        LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
        kill(client->cgi.pid, SIGTERM);
        close(client->cgi.pipe_fd);
        return 500;
      }
      client->cgi.start = std::time(NULL);
      cgi_to_client[client->cgi.pipe_fd] = client;
    }
    return -1;
  } else if (actions & EPOLLIN) {

    LOG_STREAM(DEBUG, "HELLO");
    bytes_read = read(client->cgi.pipe_fd, buffer, sizeof(buffer));
    if (bytes_read > 0) {
      client->cgi.data_received = true;
      written = 0;
      while (written < bytes_read) {
        ret = write(client->cgi.output_fd, buffer, bytes_read);
        if (ret < 0) {
          LOG_STREAM(ERROR,
                     "CGI: Write to temp file failed: " << strerror(errno));
          in_cgi_cleanup(epoll_fd, client);
          return 503;
        }
        written += ret;
      }
      return -1;
    } else if (bytes_read < 0) {
      LOG_STREAM(ERROR,
                 "CGI: Read from output pipe failed: " << strerror(errno));
      in_cgi_cleanup(epoll_fd, client);
      return 500;
    } else {
      return prepare_cgi_response(epoll_fd, client, true);
    }
  }
  return -1;
}
