#include "webserv.hpp"
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
      tokens.push_back(std::string(start, end));
      start = end + 1;
    }
    ++end;
  }
  tokens.push_back(std::string(start, end));
  return tokens;
}

// get the current working dir
std::string current_path() {
  char temp[PATH_MAX];
  int error;

  errno = 0;
  if (getcwd(temp, PATH_MAX) != 0)
    return std::string(temp);

  error = errno;
  switch (error) {
  case EACCES:
    throw std::runtime_error("Access denied");
  case ENOMEM:
    throw std::runtime_error("Insufficient storage");
  default: {
    throw std::runtime_error("Unrecognised error");
  }
  }
}

// read a file to a string
// std::string read_file(const std::string &filename) {
//   std::ifstream input_file(filename.c_str(), std::ios::binary);
//   if (!input_file) {
//     throw std::runtime_error("Error opening ");
//   }
//
//   std::ostringstream buffer;
//   buffer << input_file.rdbuf();
//   return buffer.str();
// }
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

// get sockaddr of IP4 or IP6
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET)
    return &(((struct sockaddr_in *)sa)->sin_addr);
  else
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

// print the content of addrinfo
void print_addrinfo(struct addrinfo *info) {
  for (struct addrinfo *p = info; p != NULL; p = p->ai_next) {
    printf("Address Info:\n");

    // Print address family
    printf("  ai_family: ");
    switch (p->ai_family) {
    case AF_INET:
      printf("AF_INET (IPv4)\n");
      break;
    case AF_INET6:
      printf("AF_INET6 (IPv6)\n");
      break;
    default:
      printf("Unknown (%d)\n", p->ai_family);
    }

    // Print socket type
    printf("  ai_socktype: ");
    switch (p->ai_socktype) {
    case SOCK_STREAM:
      printf("SOCK_STREAM (TCP)\n");
      break;
    case SOCK_DGRAM:
      printf("SOCK_DGRAM (UDP)\n");
      break;
    case SOCK_RAW:
      printf("SOCK_RAW (Raw)\n");
      break;
    default:
      printf("Unknown (%d)\n", p->ai_socktype);
    }

    // Print protocol
    printf("  ai_protocol: ");
    switch (p->ai_protocol) {
    case IPPROTO_TCP:
      printf("IPPROTO_TCP (TCP)\n");
      break;
    case IPPROTO_UDP:
      printf("IPPROTO_UDP (UDP)\n");
      break;
    default:
      printf("Unknown (%d)\n", p->ai_protocol);
    }

    // Print canonical name (if available)
    printf("  ai_canonname: %s\n",
           p->ai_canonname ? p->ai_canonname : "(null)");

    // Print address (IPv4 or IPv6)
    char ipstr[INET6_ADDRSTRLEN];
    void *addr;
    if (p->ai_family == AF_INET) { // IPv4
      struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
      addr = &(ipv4->sin_addr);
    } else { // IPv6
      struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)p->ai_addr;
      addr = &(ipv6->sin6_addr);
    }
    inet_ntop(p->ai_family, addr, ipstr, sizeof(ipstr));
    printf("  ai_addr: %s\n", ipstr);

    // Print address length
    printf("  ai_addrlen: %d\n", (int)p->ai_addrlen);

    printf("\n");
  }
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


void print_address_and_port(const struct sockaddr_storage &client_addr) {
  char ipstr[INET6_ADDRSTRLEN];

  if (client_addr.ss_family == AF_INET) {
    struct sockaddr_in *ipv4 = (struct sockaddr_in *)&client_addr;
    inet_ntop(AF_INET, &(ipv4->sin_addr), ipstr, sizeof ipstr);
    LOG_STREAM(INFO,
               "Address: " << ipstr << ", Port: " << ntohs(ipv4->sin_port));
  } else if (client_addr.ss_family == AF_INET6) {
    struct sockaddr_in6 *ipv6 = (struct sockaddr_in6 *)&client_addr;
    inet_ntop(AF_INET6, &(ipv6->sin6_addr), ipstr, sizeof ipstr);
    LOG_STREAM(INFO,
               "Address: " << ipstr << ", Port: " << ntohs(ipv6->sin6_port));
  }
}
