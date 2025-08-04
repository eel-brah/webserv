#include "../include/webserv.hpp"
#include <cstdio>
#include <dirent.h>

const size_t CHUNK_THRESHOLD = 1024 * 1024; // 1MB
const int chunk_size = 8192;
const size_t FIXED_BUFFER_SIZE = 1024 * 16;

bool handle_write(Client &client) {
  ssize_t sent;
  int client_fd = client.get_socket();
  size_t to_send;

  if (client.write_offset < client.response.size()) {
    to_send = std::min(FIXED_BUFFER_SIZE,
                       client.response.size() - client.write_offset);
    sent = send(client_fd, client.response.c_str() + client.write_offset,
                to_send, MSG_NOSIGNAL);
    if (sent <= 0) {
      // if (errno == EINTR)
      //   continue;
      LOG_STREAM(ERROR,
                 "send error on fd " << client_fd << ": " << strerror(errno));
      return false;
    }
    client.write_offset += sent;
    return true;
  }

  client.response.clear();
  client.write_offset = 0;
  if (client.chunk) {
    int file_fd = client.response_fd;
    char buffer[chunk_size];
    ssize_t bytes;

    if (client.chunk_offset < client.current_chunk.size()) {
      to_send = std::min(FIXED_BUFFER_SIZE,
                         client.current_chunk.size() - client.chunk_offset);
      sent = send(client_fd, client.current_chunk.c_str() + client.chunk_offset,
                  to_send, MSG_NOSIGNAL);
      if (sent < 0) {
        LOG_STREAM(ERROR, "send error (chunk) on fd " << client_fd << ": "
                                                      << strerror(errno));
        close(file_fd);
        return false;
      }
      client.chunk_offset += sent;
      return true;
    }

    client.current_chunk.clear();
    client.chunk_offset = 0;

    if (client.final_chunk_sent) {
      close(file_fd);
      client.chunk = false;
      client.final_chunk_sent = false;
      client.clear_request();
      if (client.error_code) {
        client.free_client = true;
        client.error_code = false;
      }
      return true;
    }

    bytes = read(file_fd, buffer, sizeof(buffer));
    if (bytes < 0) {
      LOG_STREAM(ERROR,
                 "read error on fd " << file_fd << ": " << strerror(errno));
      close(file_fd);
      return false;
    }

    if (bytes == 0) {
      client.current_chunk = std::string("0") + CRLF + CRLF;
      client.final_chunk_sent = true;
    } else {
      client.current_chunk =
          int_to_hex(bytes) + CRLF + std::string(buffer, bytes) + CRLF;
    }
    return true;
  }

  client.clear_request();
  if (client.error_code == true) {
    client.free_client = true;
    client.error_code = false;
  }
  return true;
}

bool is_redirect(int code) {
  return (code == 301 || code == 302 || code == 307 || code == 308);
}

void generate_response(Client &client, int file_fd, const std::string &file,
                       int status_code, std::string info = "",
                       const std::string &body_content = "") {
  std::string status_line;
  std::string headers = get_server_header() + get_date_header();
  std::string body;
  std::string response;
  std::string content;
  size_t content_size = 0;

  if (file_fd != -1) {
    struct stat st;
    if (stat(file.c_str(), &st) == -1) {
      close(file_fd);
      throw std::runtime_error("stat failed: " + std::string(strerror(errno)));
    }
    if (!S_ISREG(st.st_mode)) {
      close(file_fd);
      throw std::runtime_error("Not a regular file");
    }
    content_size = st.st_size;
  }

  status_line = generate_status_line(status_code);
  if (status_code == 405) {
    headers += get_allow_header(info);
  } else if (status_code == 201 || is_redirect(status_code)) {
    headers += get_location_header(info);
  }
  headers += get_content_type(file);

  if (content_size < CHUNK_THRESHOLD || file_fd == -1) {
    if (file_fd != -1) {
      content = read_file_to_str(file_fd, content_size);
    } else if (!body_content.empty()) {
      content = body_content;

    } else if (status_code == 201 || status_code == 204) {
      content = "";
    } else {
      content = special_response(status_code);
    }

    headers += get_content_length(content.size());
    headers += CRLF;
    body = content;

    response = status_line + headers + body;

    client.chunk = false;
    client.fill_response(response);
    close(file_fd);
  } else {
    headers += get_transfer_encoding("chunked");
    headers += CRLF;

    response = status_line + headers;
    client.fill_response(response);

    client.chunk = true;
    client.response_fd = file_fd;
    client.chunk_offset = 0;
    client.current_chunk.clear();
    client.final_chunk_sent = false;
  }
}

void send_special_response(Client &client, int status_code, std::string info) {
  std::map<int, std::string> error_pages =
      client.get_request()->server_conf->getErrorPages();
  std::map<int, std::string>::const_iterator it = error_pages.find(status_code);
  if (it != error_pages.end()) {
    int fd = open(it->second.c_str(), O_RDONLY);
    if (fd < 0) {
      LOG_STREAM(ERROR, "Error opening error page: " << strerror(errno));
      if (status_code != 500)
        send_special_response(client, 500);
      else
        generate_response(client, -1, ".html", 500);
      return;
    }
    generate_response(client, fd, it->second, status_code, info);
    return;
  }
  generate_response(client, -1, ".html", status_code, info);
}

std::string handle_file_upload(Client &client, std::string upload_store) {
  std::cerr << "body_created = " << client.get_request()->body_created
            << std::endl;
  if (!client.get_request() || client.get_request()->body.empty() ||
      !client.get_request()->body_created) {
    LOG_STREAM(ERROR, "Invalid or empty request body");
    send_special_response(client, 400);
    return "";
  }

  std::string filename;
  filename = "/file_" + int_to_string((int)(std::time(0)));
  std::string file_type = "";
  try {
    file_type = client.get_request()->get_header_by_key("content-type")->value;
    std::vector<std::string> type = split(file_type, '/');
    if (type.size() == 2) {
      if (type[1].size() > 10)
        file_type = ".raw";
      else
        file_type = "." + strip(type[1]);
    } else
      file_type = "";
  } catch (std::exception &e) {
    LOG_STREAM(WARNING, "No content-type found");
  }

  std::string path = upload_store + filename + file_type;
  if (path.length() >= PATH_MAX) {
    LOG_STREAM(ERROR, "Generated path too long: " << path);
    send_special_response(client, 500);
    return "";
  }

  // TODO: if there is no body
  int infd = open(client.get_request()->body.c_str(), O_RDONLY);
  if (infd < 0) {
    LOG_STREAM(ERROR, "Error opening input file: " << strerror(errno));
    send_special_response(client, 500);
    return "";
  }

  int outfd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (outfd < 0) {
    LOG_STREAM(ERROR,
               "Error opening output file " << path << ": " << strerror(errno));
    send_special_response(client, 500);
    close(infd);
    return "";
  }

  size_t buffer_size = 4096;
  std::vector<char> buffer(buffer_size);

  while (true) {
    ssize_t bytes_read = read(infd, buffer.data(), buffer_size);
    if (bytes_read < 0) {
      if (errno == EINTR)
        continue;
      LOG_STREAM(ERROR, "Error reading input file: " << strerror(errno));
      send_special_response(client, 500);
      close(infd);
      close(outfd);
      return "";
    }
    if (bytes_read == 0)
      break;

    ssize_t total_written = 0;
    while (total_written < bytes_read) {
      ssize_t bytes_written = write(outfd, buffer.data() + total_written,
                                    bytes_read - total_written);
      if (bytes_written < 0) {
        if (errno == EINTR)
          continue;
        LOG_STREAM(ERROR, "Error writing to output file: " << strerror(errno));
        send_special_response(client, 500);
        close(infd);
        close(outfd);
        return "";
      }
      total_written += bytes_written;
    }
  }

  if (fsync(outfd) < 0) {
    LOG_STREAM(ERROR, "Error syncing output file: " << strerror(errno));
    send_special_response(client, 500);
    close(infd);
    close(outfd);
    return "";
  }
  close(infd);
  close(outfd);
  return path;
}

std::string get_default_file(const std::vector<std::string> &index,
                             std::string file_path) {
  if (file_path[file_path.size() - 1] != '/')
    file_path += "/";
  if (!index.empty()) {
    struct stat buf;
    std::string::size_type base_len = file_path.length();
    for (std::vector<std::string>::const_iterator it = index.begin();
         it != index.end(); ++it) {
      file_path += *it;
      if (stat(file_path.c_str(), &buf) == 0)
        return file_path;
      file_path.resize(base_len);
    }
  }
  return "";
}
bool is_dir(const std::string &path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    return false;
  }
  return S_ISDIR(info.st_mode);
}

const LocationConfig *get_location(const std::vector<LocationConfig> &locations,
                                   const std::string &path) {
  const LocationConfig *exact = NULL;
  const LocationConfig *best_prefix = NULL;
  size_t longest_prefix = 0;

  for (std::vector<LocationConfig>::const_iterator it = locations.begin();
       it != locations.end(); ++it) {
    std::string loc_path = strip(it->path);

    const std::string &candidate = path;

    if (loc_path.substr(0, 2) == "= ") {
      std::string exact_path = loc_path.substr(2);
      if (candidate == exact_path) {
        exact = &*it;
        goto done;
      }
    } else {
      if (candidate.compare(0, loc_path.length(), loc_path) == 0 &&
          (loc_path[loc_path.size() - 1] == '/' ||
           candidate.size() == loc_path.size() ||
           (candidate.size() > loc_path.size() &&
            candidate[loc_path.size()] == '/'))) {
        if (!best_prefix || loc_path.length() > longest_prefix) {
          best_prefix = &*it;
          longest_prefix = loc_path.length();
        }
      }
    }
  }

done:
  if (exact)
    return exact;
  if (best_prefix)
    return best_prefix;
  return NULL;
}

std::string join_vec(const std::vector<std::string> &vec) {
  std::string result;
  std::vector<std::string>::const_iterator it = vec.begin();

  if (it != vec.end()) {
    result = *it;
    ++it;
  }
  for (; it != vec.end(); ++it) {
    result += " " + *it;
  }

  return result;
}
const char *month_names[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                             "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
std::string replace_root(const std::string &root, const std::string &path) {
  std::string new_path = path;
  if (root.size() <= path.size() && path.compare(0, root.size(), root) == 0)
    new_path.replace(0, root.size(), "/");
  return new_path;
}
std::string format_time(const std::time_t &t) {
  std::tm *tm_info = std::localtime(&t);
  if (!tm_info)
    return "-";

  std::string day = int_to_string(tm_info->tm_mday);
  std::string month = month_names[tm_info->tm_mon];
  std::string year = int_to_string(tm_info->tm_year + 1900);

  std::string hour =
      (tm_info->tm_hour < 10 ? "0" : "") + int_to_string(tm_info->tm_hour);
  std::string min =
      (tm_info->tm_min < 10 ? "0" : "") + int_to_string(tm_info->tm_min);

  return day + "-" + month + "-" + year + " " + hour + ":" + min;
}

std::string format_size(off_t size) {
  if (size < 0)
    return "-";

  if (size > 1024) {
    double kb = size / 1024.0;
    std::ostringstream oss;
    oss.precision(1);
    oss.setf(std::ios::fixed);
    oss << kb << "K";
    return oss.str();
  } else {
    return long_to_string(size);
  }
}

std::string get_dir_listing(const std::string &root, const std::string &path) {
  std::string html = "<html><head><title>Index of " + replace_root(root, path) +
                     "</title></head><body><h1>Index of " +
                     replace_root(root, path) + "</h1><pre>\n";
  DIR *dir = opendir(path.c_str());
  if (!dir) {
    LOG_STREAM(ERROR, "Fail to open " << path << ": " << strerror(errno));
    return "";
  }

  html += "<a href=\"../\">../</a>\n";
  struct dirent *entry;
  std::string size_str;
  struct stat st;
  int pad_spaces;
  while ((entry = readdir(dir))) {
    std::string name = entry->d_name;
    if (name == "." || name == "..")
      continue;

    if (stat(join_paths(path, name).c_str(), &st) != 0) {
      LOG_STREAM(WARNING, "stat failed in dir listing: " << strerror(errno));
      continue;
    }

    size_str = "-";
    if (!S_ISDIR(st.st_mode)) {
      size_str = format_size(st.st_size);
    }

    html += "<a href=\"" + name + (S_ISDIR(st.st_mode) ? "/" : "") + "\">" +
            name + (S_ISDIR(st.st_mode) ? "/" : "") + "</a>";

    pad_spaces = 40 - (int)name.length();
    if (pad_spaces < 1)
      pad_spaces = 1;
    html += std::string(pad_spaces, ' ') + size_str + "  " +
            format_time(st.st_mtime) + "\n";
  }
  html += "</pre></body></html>";
  closedir(dir);
  return html;
}

bool is_method_allowed(const std::vector<HTTP_METHOD> &a, HTTP_METHOD b,
                       Client &client,
                       const std::vector<std::string> &allowed_methods) {

  if (find_in_vec(a, b) == -1) {
    send_special_response(client, 405, join_vec(allowed_methods));
    return false;
  }
  return true;
}
int can_delete_file(const std::string &filepath) {
  struct stat file_info;

  if (stat(filepath.c_str(), &file_info) != 0) {
    if (errno == ENOENT) {
      LOG_STREAM(INFO, "File does not exist: " << filepath);
      return 404;
    } else {
      LOG_STREAM(INFO, "Error accessing file: " << filepath);
      return 500;
    }
  }

  // if (S_ISDIR(file_info.st_mode)) {
  //   LOG_STREAM(INFO, "Can't delete a directory: " << filepath);
  //   return false;
  // }
  if (!S_ISREG(file_info.st_mode)) {
    LOG_STREAM(INFO, "Path is not a regular file: " << filepath);
    return 403;
  }

  if (access(filepath.c_str(), W_OK) == 0) {
    return 0;
  } else {
    LOG_STREAM(INFO, "No write permission for file: " << filepath);
    return 403;
  }
}

void process_request(Client &client) {
  HttpRequest *request = client.get_request();
  HTTP_METHOD method = request->get_method();
  ServerConfig *server_conf = client.get_request()->server_conf;
  std::string request_path = request->get_path().get_path();

  const LocationConfig *location =
      get_location(server_conf->getLocations(), request_path);
  if (!location) {
    send_special_response(client, 404);
    return;
  }

  std::string path = join_paths(location->root, request_path);

  // if (location->path != request_path && path[path.size() - 1] != '/' &&
  if (path[path.size() - 1] != '/' && is_dir(path)) {
    send_special_response(client, 301, request_path + "/");
    return;
  }

  // TODO: alias here

  if (is_redirect(location->redirect_code)) {
    // TODO: return 404;
    send_special_response(client, location->redirect_code,
                          location->redirect_url);
    return;
  }
  if (method == GET) {
    // NOTE:401 Unauthorized  / 406 Not Acceptable / 416 Requested Range Not
    // Satisfiable / 417 Expectation Failed
    if (!is_method_allowed(location->allowed_methods2, GET, client,
                           location->allowed_methods))
      return;

    if (!location->cgi_ext.empty()) {
      if (!client.executeCGI(*server_conf, path, location))
          send_special_response(client, 500);
      return;
    }
    if (!location->alias.empty()) {
      std::string tmp = request_path;
      tmp.erase(tmp.find(location->path), location->path.length());
      path = join_paths(location->alias, tmp);
    }
    if (is_dir(path)) {
      std::string new_path = get_default_file(location->index, path);
      if (new_path.empty()) {
        if (location->autoindex && (request_path == location->path ||
                                    request_path == location->path + "/")) {
          std::string dir_listing = get_dir_listing(location->root, path);

          if (dir_listing.empty())
            send_special_response(client, 500);
          generate_response(client, -1, ".html", 200, "", dir_listing);
        } else
          send_special_response(client, 403);
        return;
      }
      path = new_path;
    }

    int fd = open(path.c_str(), O_RDONLY | O_NONBLOCK);
    int error_code;
    if (fd == -1) {
      if (errno == ENOENT || errno == ENOTDIR)
        error_code = 404;
      else
        error_code = 500;
      LOG_STREAM(ERROR, "Open: " << strerror(errno));
      send_special_response(client, error_code);
    } else {
      // NOTE: 206 Partial Content: delivering part of the resource due to a
      // Range header in the GET request. The response includes a Content-Range
      // header.
      generate_response(client, fd, path, 200);
    }
  } else if (method == POST) {
    if (!is_method_allowed(location->allowed_methods2, POST, client,
                           location->allowed_methods))
      return;
    if (!location->cgi_ext.empty()) {
      if (!client.executeCGI(*server_conf, path, location))
          send_special_response(client, 500);
      return;
    }
    if (!location->upload_store.empty()) {
      std::string file_path =
          handle_file_upload(client, location->upload_store);
      if (!file_path.empty())
        generate_response(client, -1, "", 201, file_path);
    } else
      send_special_response(client, 405, join_vec(location->allowed_methods));

  } else if (method == DELETE) {
    if (!is_method_allowed(location->allowed_methods2, DELETE, client,
                           location->allowed_methods))
      return;
    int code = can_delete_file(path);
    if (code) {
      send_special_response(client, code);
      return;
    }
    if (remove(path.c_str()) == -1) {
      LOG_STREAM(ERROR,
                 "Fail to remove file: " << path << ": " << strerror(errno));
      send_special_response(client, 500);
      return;
    }
    send_special_response(client, 204);
  } else {
    send_special_response(client, 405, join_vec(location->allowed_methods));
  }
}
