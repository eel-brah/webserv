#include "ClientPool.hpp"
#include "errors.hpp"
#include "parser.hpp"
#include "webserv.hpp"

int set_nonblocking(int server_fd) {
  int flags = fcntl(server_fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
}

void free_client(int epoll_fd, Client *client,
                 std::map<int, Client *> *fd_to_client, ClientPool *pool) {

  LOG(INFO, "Free Client");
  int fd = client->get_socket();
  fd_to_client->erase(fd);
  if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1)
    LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
  try {
    pool->deallocate(client);
  } catch (std::exception &e) {
    LOG_STREAM(WARNING, "ClientPool: " << e.what());
  }
}

bool handle_client(int epoll_fd, Client &client, uint32_t actions) {
  int status_code = 0;

  if (actions & EPOLLIN) {
    // Read data from client and process request, then prepare a response:
    try {
      // NOTE: 100 Continue && 101 Switching Protocols
      if (!client.parse_loop())
        return false;
    } catch (ParsingError &e) {
      status_code = static_cast<PARSING_ERROR>(e.get_type());
    } catch (std::exception &e) {
      // TODO: handle this case
      throw std::runtime_error("uncached exception!!");
    }
    try {
      if (status_code)
        error_response(client, status_code);
      else
        generate_response(client);
    } catch (std::exception &e) {
      LOG_STREAM(ERROR, "Generating response failed: " << e.what());
      error_response(client, 500);
    }
    if (!handle_write(client))
      return false;
  }

  if (actions & EPOLLOUT) {
    // write the cilent
    if (!handle_write(client))
      return false;
  }

  if (actions & (EPOLLHUP | EPOLLERR)) {
    LOG(ERROR, "Client disconnected or error\n");
    return false;
  }
  return true;
}

class ServerInfo {
private:
  int fd;

  ServerInfo();

public:
  ServerInfo(const int fd) : fd(fd) {}
  ~ServerInfo() { close(fd); }
  ServerInfo(const ServerInfo &other) { *this = other; }
  ServerInfo &operator=(const ServerInfo &other) {
    if (this != &other) {
      fd = other.fd;
    }
    return *this;
  }

  int get_fd() { return fd; }
};

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
    LOG_STREAM(ERROR, "getaddrinfo error: " << gai_strerror(status));
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
      LOG_STREAM(ERROR, "socket: " << strerror(errno));
      continue;
    }
    if (set_nonblocking(server_fd) == -1) {
      LOG_STREAM(ERROR, "fcntl: " << strerror(errno));
      close(server_fd);
      freeaddrinfo(servinfo);
      return 1;
    }
    // Allow port reuse to avoid "Address already in use" errors
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      LOG_STREAM(ERROR, "setsockopt: " << strerror(errno));
      close(server_fd);
      freeaddrinfo(servinfo);
      return 1;
    }
    if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_fd);
      LOG_STREAM(ERROR, "bind: " << strerror(errno));
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo);

  ServerInfo server(server_fd);

  if (p == NULL) {
    LOG_STREAM(ERROR, "server: failed to bind");
    return 1;
  }

  if (listen(server.get_fd(), SOMAXCONN)) {
    LOG_STREAM(ERROR, "listen: " << strerror(errno));
    return 1;
  }

  int epoll_fd;
  struct epoll_event ev, events[MAX_EVENTS];

  // Create epoll instance for event-driven I/O
  epoll_fd = epoll_create(1);
  if (epoll_fd == -1) {
    LOG_STREAM(ERROR, "epoll_create: " << strerror(errno));
    return 1;
  }

  // Configure epoll to monitor server socket for incoming connections
  ev.events = EPOLLIN;
  ev.data.fd = server.get_fd();
  if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server.get_fd(), &ev) == -1) {
    LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
    close(epoll_fd);
    return 1;
  }

  LOG_STREAM(INFO, "Server is listening on " << PORT);

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
    LOG_STREAM(ERROR, "Memory allocation failed: " << e.what());
    close(epoll_fd);
    return 1;
  }

  for (;;) {
    // Wait for events on monitored file descriptors
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      LOG_STREAM(ERROR, "epoll_wait: " << strerror(errno));
      continue;
    }

    for (int i = 0; i < nfds; i++) {
      if (events[i].data.fd == server.get_fd()) {
        addr_size = sizeof client_addr;
        client_fd = accept(server.get_fd(), (struct sockaddr *)&client_addr,
                           &addr_size);
        if (client_fd == -1) {
          LOG_STREAM(ERROR, "accept: " << strerror(errno));
          continue;
        }
        if (set_nonblocking(client_fd) == -1) {
          LOG_STREAM(ERROR, "fcntl: " << strerror(errno));
          close(client_fd);
          continue;
        }

        // Add client socket to epoll for edge-triggered monitoring
        ev.events = EPOLLIN | EPOLLET | EPOLLOUT;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
          LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
          close(client_fd);
          continue;
        }

        // TODO: handle if no slot available / 503 Service Unavailable
        client = pool->allocate(client_fd);
        if (!client) {
          LOG_STREAM(ERROR, "No free client slots available");
          close(client_fd);
          continue;
        }
        (*fd_to_client)[client_fd] = client;

        // Convert client address to string and log connection
        inet_ntop(client_addr.ss_family,
                  get_in_addr((struct sockaddr *)&client_addr), ipstr,
                  sizeof ipstr);
        LOG_STREAM(INFO, "server: got connection from " << ipstr);
      } else {
        // Handle communication with an existing clients
        int client_fd = events[i].data.fd;
        std::map<int, Client *>::iterator it = fd_to_client->find(client_fd);
        if (it != fd_to_client->end()) {
          client = it->second;
          if (!handle_client(epoll_fd, *client, events[i].events)) {
            free_client(epoll_fd, client, fd_to_client, pool);
          }
        } else {
          LOG_STREAM(ERROR, "Unknown fd " << client_fd);
        }
      }
    }
  }
  delete pool;
  delete fd_to_client;
  close(epoll_fd);
  return 0;
}
