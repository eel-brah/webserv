#include "webserv.hpp"
#include <algorithm>

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
std::string read_file_to_str(const char *filename) {
  std::ifstream input_file(filename, std::ios::binary);
  if (!input_file) {
    std::cerr << "Error opening " << filename << std::endl;
    std::exit(1);
  }

  input_file.seekg(0, std::ios::end);
  std::streamsize size = input_file.tellg();
  input_file.seekg(0, std::ios::beg);

  if (size <= 0)
    return "";

  std::vector<char> buffer(size);
  if (!input_file.read(&buffer[0], size)) {
    std::cerr << "Could not read file " << filename << std::endl;
    std::exit(1);
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

// wait for dead children
void sigchld_handler(int s) {
  int saved_errno = errno;
  while (waitpid(-1, NULL, WNOHANG) > 0)
    ;
  errno = saved_errno;
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
