#include "../include/webserv.hpp"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>

// split string with del
std::vector<std::string> split(const std::string &str, char del) {
  std::vector<std::string> tokens;
  std::string token;

  for (std::string::size_type i = 0; i < str.length(); ++i) {
    if (str[i] == del) {
      if (!token.empty())
        tokens.push_back(token);
      token.clear();
    } else {
      token += str[i];
    }
  }
  tokens.push_back(token);
  return tokens;
}

// split char * with del
std::vector<std::string> split(const char *str, char del) {
  std::vector<std::string> tokens;
  const char *start = str;
  const char *end = str;

  if (!str)
    return tokens;
  while (*end != '\0') {
    if (*end == del) {
      if (start != end)
        tokens.push_back(std::string(start, end));
      start = end + 1;
    }
    ++end;
  }
  tokens.push_back(std::string(start, end));
  return tokens;
}

std::string read_file_to_str(const char *filename) {
  struct stat st;
  if (stat(filename, &st) == -1) {
    throw std::runtime_error("stat failed: " + std::string(strerror(errno)));
  }

  if (!S_ISREG(st.st_mode)) {
    throw std::runtime_error("Not a regular file: " + std::string(filename));
  }

  int fd = open(filename, O_RDONLY);
  if (fd == -1) {
    throw std::runtime_error("open failed: " + std::string(strerror(errno)));
  }

  std::string content(st.st_size, '\0');
  ssize_t total_read = 0;

  while (total_read < st.st_size) {
    ssize_t bytes = read(fd, &content[total_read], st.st_size - total_read);
    if (bytes < 0) {
      close(fd);
      throw std::runtime_error("read failed: " + std::string(strerror(errno)));
    }
    if (bytes == 0) {
      break;
    }
    total_read += bytes;
  }

  close(fd);
  return content;
}

std::string read_file_to_str(int fd, size_t size) {
  if (size == 0) {
    return "";
  }

  std::string content(size, '\0');
  size_t total_read = 0;
  while (total_read < size) {
    ssize_t bytes = read(fd, &content[total_read], size - total_read);
    if (bytes < 0) {
      throw std::runtime_error("read failed: " + std::string(strerror(errno)));
    }
    if (bytes == 0) {
      break;
    }
    total_read += bytes;
  }

  return content;
}
std::string read_file_to_str(const std::string &filename) {
  std::ifstream input_file(filename.c_str(), std::ios::binary);
  if (!input_file) {
    throw std::runtime_error("Error opening ");
  }

  input_file.seekg(0, std::ios::end);
  std::streamsize size = input_file.tellg();
  input_file.seekg(0, std::ios::beg);

  if (size <= 0)
    return "";

  std::vector<char> buffer(size);
  if (!input_file.read(&buffer[0], size)) {
    throw std::runtime_error("Could not read file");
  }
  return std::string(buffer.begin(), buffer.end());
}

std::string get_ip(const struct sockaddr *sa) {
  std::stringstream oss;

  if (sa->sa_family == AF_INET) {
    struct sockaddr_in *sin = (struct sockaddr_in *)sa;
    uint32_t ip = ntohl(sin->sin_addr.s_addr);
    oss << ((ip >> 24) & 0xFF) << "." << ((ip >> 16) & 0xFF) << "."
        << ((ip >> 8) & 0xFF) << "." << (ip & 0xFF);
  } else if (sa->sa_family == AF_INET6) {
    struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sa;
    const uint8_t *addr = (const uint8_t *)&sin6->sin6_addr;
    for (int i = 0; i < 8; ++i) {
      if (i > 0)
        oss << ":";
      oss << std::hex << ((addr[i * 2] << 8) | addr[i * 2 + 1]);
    }
  } else {
    return "";
  }
  return oss.str();
}

void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET)
    return &(((struct sockaddr_in *)sa)->sin_addr);
  else
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int count_digits(unsigned int nb) {
  if (nb == 0)
    return 1;
  int len = 0;
  while (nb) {
    nb /= 10;
    len++;
  }
  return (len);
}

std::string int_to_string(int num) {
  if (num == 0)
    return "0";

  bool sign = 0;
  unsigned int abs_num = num;
  if (num < 0) {
    abs_num *= -1;
    sign = 1;
  }

  std::string nbr;
  while (abs_num) {
    nbr.push_back(abs_num % 10 + '0');
    abs_num /= 10;
  }
  if (sign)
    nbr.push_back('-');
  std::reverse(nbr.begin(), nbr.end());

  return nbr;
}

std::string long_to_string(long num) {
  if (num == 0)
    return "0";

  bool sign = false;
  unsigned long abs_num = static_cast<unsigned long>(num);
  if (num < 0) {
    abs_num *= -1;
    sign = true;
  }

  std::string nbr;
  while (abs_num) {
    nbr.push_back((abs_num % 10) + '0');
    abs_num /= 10;
  }

  if (sign)
    nbr.push_back('-');

  std::reverse(nbr.begin(), nbr.end());
  return nbr;
}

bool ends_with(const std::string &str, const std::string &suffix) {
  if (str.length() < suffix.length()) {
    return false;
  }
  return str.substr(str.length() - suffix.length()) == suffix;
}

long get_file_size(const std::string &filepath) {
  struct stat file_stat;
  if (stat(filepath.c_str(), &file_stat) != 0) {
    LOG_STREAM(ERROR, "Error: Unable to stat file " << filepath);
    return -1;
  }
  return file_stat.st_size;
}
std::string strip(const std::string &s) {
  size_t start = 0;
  while (start < s.length() && std::isspace(s[start])) {
    ++start;
  }

  size_t end = s.length();
  while (end > start && std::isspace(s[end - 1])) {
    --end;
  }

  return s.substr(start, end - start);
}

int set_nonblocking(int server_fd) {
  int flags = fcntl(server_fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(server_fd, F_SETFL, flags | O_NONBLOCK | FD_CLOEXEC);
}

void discard_socket_buffer(int client_fd) {
  char buffer[4096];
  int bytes_read;

  while (true) {
    bytes_read = recv(client_fd, buffer, sizeof(buffer), MSG_DONTWAIT);
    if (bytes_read > 0) {
      continue;
    } else if (bytes_read <= 0) {
      break;
    }
  }
}

bool is_dir(const std::string &path) {
  struct stat info;
  if (stat(path.c_str(), &info) != 0) {
    return false;
  }
  return S_ISDIR(info.st_mode);
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

const char* method_to_string(HTTP_METHOD method) {
    switch (method) {
        case GET:    return "GET";
        case POST:   return "POST";
        case DELETE: return "DELETE";
        default:     return "Unknown Method";
    }
}
