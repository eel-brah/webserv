#include "errors.hpp"
#include "parser.hpp"
#include "webserv.hpp"
#include <stdexcept>

#define MAX_CLIENTS 1020

class ClientPool {
private:
  enum { MAX = MAX_CLIENTS };
  char buffer[MAX * sizeof(Client)];
  std::vector<int> freeList;

public:
  ClientPool() {
    for (int i = 0; i < MAX; ++i) {
      freeList.push_back(i);
    }
  }

  Client *allocate(int fd) {
    if (freeList.empty())
      return 0;

    int idx = freeList.back();
    freeList.pop_back();

    return new (buffer + idx * sizeof(Client)) Client(fd);
  }

  void deallocate(Client *obj) {
    if (!obj)
      return;

    obj->~Client();

    int idx = (reinterpret_cast<char *>(obj) - buffer) / sizeof(Client);

    if (!(idx >= 0 && idx < MAX)) {
      throw std::runtime_error("Invalid client");
    }

    freeList.push_back(idx);
  }

  Client *get(int idx) {
    if (idx < 0 || idx >= MAX)
      return nullptr;
    return reinterpret_cast<Client *>(buffer + idx * sizeof(Client));
  }
};

int set_nonblocking(int server_fd) {
  int flags = fcntl(server_fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
}

void handle_write(int epoll_fd, Client &client) {
  while (client.write_offset < client.response.size()) {
    ssize_t sent =
        send(client.get_socket(), client.response.c_str() + client.write_offset,
             client.response.size() - client.write_offset, 0);
    if (sent < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // Socket not ready to send more, wait for next EPOLLOUT
        return;
        // else if (errno == EINTR) {
      } else {
        std::cerr << "send error on fd " << client.get_socket() << ": "
                  << strerror(errno) << std::endl;
        // TODO: close the client
        // client.~Client();
        // close(client.get_socket());
        // client = Client(-1);
        return;
      }
    }
    client.write_offset += sent;
  }

  client.response.clear();
  client.write_offset = 0;
  client.clear_request();
}

void free_client(Client *client, std::map<int, Client *> *fd_to_client,
                 ClientPool *pool) {
  int fd = client->get_socket();
  fd_to_client->erase(fd);
  pool->deallocate(client);
}

// Function to handle the communication with each client
bool handle_client(int epoll_fd, Client &client, uint32_t actions) {
  static int i;

  if (actions & EPOLLIN) {
    // NOTE: Read data from client and process request, then prepare a response:
    try {
      if (!client.parse_loop())
        return false;
    } catch (ParsingError &e) {
      switch (e.get_type()) {
      case BAD_REQUEST:
        std::cout << "<--- error ---> " << "BAD_REQUEST " << e.get_metadata()
                  << std::endl;
        break;
      case METHOD_NOT_ALLOWED:
        std::cout << "<--- error ---> " << "METHOD_NOT_ALLOWED "
                  << e.get_metadata() << std::endl;
        break;
      case METHOD_NOT_IMPLEMENTED:
        std::cout << "<--- error ---> " << "METHOD_NOT_IMPLEMENTED "
                  << e.get_metadata() << std::endl;
        break;
      case LONG_HEADER:
        std::cout << "<--- error ---> " << "LONG_HEADER " << e.get_metadata()
                  << std::endl;
        break;
      case HTTP_VERSION_NOT_SUPPORTED:
        std::cout << "<--- error ---> " << "HTTP_VERSION_NOT_SUPPORTED "
                  << e.get_metadata() << std::endl;
        break;
      default:
        throw std::runtime_error("uncached exception!!");
        break;
      }
    }
    handle_response(client, client.get_socket(), client.get_request());
    handle_write(epoll_fd, client);
  }

  if (actions & EPOLLOUT) {
    // NOTE: write the cilent
    handle_write(epoll_fd, client);
  }

  if (actions & (EPOLLHUP | EPOLLERR)) {
    std::cerr << "Client disconnected or error\n";
    return false;
  }
  return true;
}

int start_server() {
  int status;
  struct addrinfo hints, *servinfo;

  // Initialize hints structure and set socket preferences
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // Get address information for binding
  if ((status = getaddrinfo(NULL, PORT, &hints, &servinfo)) != 0) {
    std::cerr << "getaddrinfo error: " << gai_strerror(status) << std::endl;
    return 1;
  }

  // print_addrinfo(servinfo);

  struct addrinfo *p;
  int server_fd;
  int yes = 1;
  // Iterate through address info results to create and bind socket
  for (p = servinfo; p != NULL; p = p->ai_next) {
    server_fd = socket(servinfo->ai_family, servinfo->ai_socktype,
                       servinfo->ai_protocol);
    if (server_fd == -1) {
      std::cerr << "socket: " << strerror(errno) << std::endl;
      continue;
    }
    if (set_nonblocking(server_fd) == -1) {
      std::cerr << "fcntl: " << strerror(errno) << std::endl;
      return 1;
    }
    // Allow port reuse to avoid "Address already in use" errors
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      std::cerr << "setsockopt: " << strerror(errno) << std::endl;
      return 1;
    }
    if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_fd);
      std::cerr << "bind: " << strerror(errno) << std::endl;
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo);

  if (p == NULL) {
    std::cerr << "server: failed to bind" << std::endl;
    return 1;
  }

  if (listen(server_fd, SOMAXCONN)) {
    std::cerr << "listen: " << strerror(errno) << std::endl;
    return 1;
  }

  int epoll_fd;
  struct epoll_event ev, events[MAX_EVENTS];

  // Create epoll instance for event-driven I/O
  epoll_fd = epoll_create(1);
  if (epoll_fd == -1) {
    std::cerr << "epoll_create: " << strerror(errno) << std::endl;
    return 1;
  }

  // Configure epoll to monitor server socket for incoming connections
  ev.events = EPOLLIN;
  ev.data.fd = server_fd;
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
    std::cerr << "epoll_ctl: " << strerror(errno) << std::endl;
    return 1;
  }

  std::cout << "Server is listening on " << PORT << std::endl;

  struct sockaddr_storage client_addr;
  int client_fd;
  socklen_t addr_size;
  char ipstr[INET6_ADDRSTRLEN];
  int nfds;

  ClientPool *pool;
  std::map<int, Client *> *fd_to_client;
  Client *client;
  try {
    pool = new ClientPool();
    fd_to_client = new std::map<int, Client *>;
  } catch (const std::bad_alloc &e) {
    std::cerr << "Memory allocation failed: " << e.what() << std::endl;
    return 1;
  }

  for (;;) {
    // Wait for events on monitored file descriptors
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      std::cerr << "epoll_wait: " << strerror(errno) << std::endl;
      continue;
    }

    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == server_fd) {
        addr_size = sizeof client_addr;
        client_fd =
            accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_fd == -1) {
          std::cerr << "accept: " << strerror(errno) << std::endl;
          continue;
        }
        if (set_nonblocking(client_fd) == -1) {
          std::cerr << "fcntl: " << strerror(errno) << std::endl;
          close(client_fd);
          continue;
        }

        // Add client socket to epoll for edge-triggered monitoring
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
          std::cerr << "epoll_ctl: " << strerror(errno) << std::endl;
          close(client_fd);
          continue;
        }

        // TODO: handle if no slot available
        client = pool->allocate(client_fd);
        if (!client) {
          std::cerr << "No free client slots available" << std::endl;
          close(client_fd);
          continue;
        }
        (*fd_to_client)[client_fd] = client;

        // Convert client address to string and log connection
        inet_ntop(client_addr.ss_family,
                  get_in_addr((struct sockaddr *)&client_addr), ipstr,
                  sizeof ipstr);
        std::cout << "server: got connection from " << ipstr << std::endl;
      } else {
        // Handle communication with an existing clients
        int client_fd = events[i].data.fd;
        std::map<int, Client *>::iterator it = fd_to_client->find(client_fd);
        if (it != fd_to_client->end()) {
          client = it->second;
          if (!handle_client(epoll_fd, *client, events[i].events))
          {
            std::cout << "remove" << std::endl;
            free_client(client, fd_to_client, pool);
          }
        } else {
          std::cerr << "Unknown fd " << client_fd << std::endl;
        }
      }
    }
  }
  delete pool;
  delete fd_to_client;
  close(server_fd);
  close(epoll_fd);
  return 0;
}
