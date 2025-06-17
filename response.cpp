#include "webserv.hpp"

const size_t CHUNK_THRESHOLD = 1024 * 1024; // 1MB
const int chunk_size = 8192;

#define ROOT current_path() + "/root"

bool handle_write(Client &client) {
  ssize_t sent;
  int client_fd = client.get_socket();

  while (client.write_offset < client.response.size()) {
    sent = send(client_fd, client.response.c_str() + client.write_offset,
                client.response.size() - client.write_offset, 0);
    if (sent < 0) {
      if (errno == EINTR)
        continue;
      if (errno == EAGAIN || errno == EWOULDBLOCK)
        return true;
      LOG_STREAM(ERROR,
                 "send error on fd " << client_fd << ": " << strerror(errno));
      return false;
    }
    client.write_offset += sent;
  }

  client.response.clear();
  client.write_offset = 0;

  if (client.chunk) {
    int file_fd = client.response_fd;
    char buffer[chunk_size];
    ssize_t bytes;

    while (true) {
      while (client.chunk_offset < client.current_chunk.size()) {
        sent =
            send(client_fd, client.current_chunk.c_str() + client.chunk_offset,
                 client.current_chunk.size() - client.chunk_offset, 0);
        if (sent < 0) {
          if (errno == EINTR)
            continue;
          if (errno == EAGAIN || errno == EWOULDBLOCK)
            return true;
          LOG_STREAM(ERROR, "send error (chunk) on fd " << client_fd << ": "
                                                        << strerror(errno));
          return false;
        }
        client.chunk_offset += sent;
      }

      client.current_chunk.clear();
      client.chunk_offset = 0;

      if (client.final_chunk_sent) {
        break;
      }

      bytes = read(file_fd, buffer, sizeof(buffer));
      if (bytes < 0) {
        if (errno == EINTR)
          continue;
        LOG_STREAM(ERROR,
                   "read error on fd " << file_fd << ": " << strerror(errno));
        return false;
      }

      if (bytes == 0) {
        client.current_chunk = std::string("0") + CRLF + CRLF;
        client.final_chunk_sent = true;
        continue;
      }

      client.current_chunk =
          int_to_hex(bytes) + CRLF + std::string(buffer, bytes) + CRLF;
    }

    client.chunk = false;
    client.current_chunk.clear();
    client.chunk_offset = 0;
    client.final_chunk_sent = false;
  }

  client.clear_request();
  return true;
}

void generate_response(Client &client, int file_fd, const std::string &file,
                       int status_code) {
  std::string status_line;
  std::string headers = get_server_header() + get_date_header();
  std::string body;
  std::string response;
  std::string content;
  size_t content_size = 0;

  if (file_fd != -1) {
    struct stat st;
    if (fstat(file_fd, &st) == -1) {
      throw std::runtime_error("fstat failed: " + std::string(strerror(errno)));
    }
    if (!S_ISREG(st.st_mode)) {
      throw std::runtime_error("fd does not refer to a regular file");
    }
    content_size = st.st_size;
  }

  status_line = generate_status_line(status_code);
  if (status_code == 405) {
    headers += get_allow_header("GET POST");
  }
  headers += get_content_type(file);

  if (content_size < CHUNK_THRESHOLD || file_fd == -1) {
    if (file_fd != -1) {
      content = read_file_to_str(file_fd, content_size);
    } else if (status_code == 201) {
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

void send_error(Client &client, int status_code) {
  // TODO: check if status code have a costume error page
  generate_response(client, -1, ".html", status_code);
}

// #define ERRORS ROOT + "/errors"
// void generate_response_error(Client &client, int status_code) {
//   std::string file = ERRORS + "/" + int_to_string(status_code) + ".html";
//   int fd = open(file.c_str(), O_RDONLY | O_NONBLOCK);
//   if (fd == -1) {
//     send_error(client, status_code);
//     return;
//   }
//   generate_response(client, fd, file, status_code);
// }

bool handle_file_upload(Client &client) {
  if (!client.get_request() || client.get_request()->body.empty()) {
    LOG_STREAM(ERROR, "Invalid or empty request body");
    send_error(client, 400);
    return false;
  }

  std::string filename;
  filename = "/file_" + int_to_string((int)(std::time(0)));

  std::string path = ROOT + filename;
  if (path.length() >= PATH_MAX) {
    LOG_STREAM(ERROR, "Generated path too long: " << path);
    send_error(client, 500);
    return false;
  }

  // TODO: if there is no body
  int infd = open(client.get_request()->body.c_str(), O_RDONLY);
  if (infd < 0) {
    LOG_STREAM(ERROR, "Error opening input file: " << strerror(errno));
    send_error(client, 500);
    return false;
  }

  int outfd = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (outfd < 0) {
    LOG_STREAM(ERROR,
               "Error opening output file " << path << ": " << strerror(errno));
    send_error(client, 500);
    return false;
  }

  size_t buffer_size = 4096;
  std::vector<char> buffer(buffer_size);

  while (true) {
    ssize_t bytes_read = read(infd, buffer.data(), buffer_size);
    if (bytes_read < 0) {
      if (errno == EINTR)
        continue;
      LOG_STREAM(ERROR, "Error reading input file: " << strerror(errno));
      send_error(client, 500);
      return false;
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
        send_error(client, 500);
        return false;
      }
      total_written += bytes_written;
    }
  }

  if (fsync(outfd) < 0) {
    LOG_STREAM(ERROR, "Error syncing output file: " << strerror(errno));
    send_error(client, 500);
    return false;
  }
  return true;
}

// poll_fds[findPollIndex(client_fd)].events |= POLLOUT;
void process_request(Client &client) {
  HttpRequest *request = client.get_request();
  HTTP_METHOD method = request->get_method();

  std::string file = get_file_path(request->get_path().get_path());

  if (method == GET) {
    // NOTE:401 Unauthorized / 405 Method Not Allowed / 406 Not Acceptable / 403
    // Forbidden / 416 Requested Range Not Satisfiable / 417 Expectation Failed
    int fd = open(file.c_str(), O_RDONLY | O_NONBLOCK);
    int error_code;
    if (fd == -1) {
      if (errno == ENOENT)
        error_code = 404;
      // else if (errno == EACCES)
      //   error_code = 403;
      else
        error_code = 500;
      send_error(client, error_code);
    } else {
      // NOTE: 206 Partial Content: delivering part of the resource due to a
      // Range header in the GET request. The response includes a Content-Range
      // header.
      generate_response(client, fd, file, 200);
    }
  } else if (method == POST) {
    if (handle_file_upload(client))
      generate_response(client, -1, "", 201);
  } else {
    send_error(client, 405);
  }
}
