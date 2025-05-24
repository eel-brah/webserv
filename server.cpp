#include "parser.hpp"
#include "webserv.hpp"

int set_nonblocking(int server_fd) {
  int flags = fcntl(server_fd, F_GETFL, 0);
  if (flags == -1)
    return -1;
  return fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);
}

// Function to handle the communication with each client
void handle_client(Client client) {
  // char buffer[1024];

  // Receive data from the client
  // ssize_t bytes_received = client.recv(buffer, sizeof(buffer));
  // std::cout << bytes_received << std::endl;
  while (client.parse_loop()) {
  }
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

  Client clients[MAX_EVENTS];
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
        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
          std::cerr << "epoll_ctl: " << strerror(errno) << std::endl;
          close(client_fd);
          continue;
        }

        // TODO: handle alocation error
        // TODO: and free
        new (&clients[i]) Client(client_fd);

        // Convert client address to string and log connection
        inet_ntop(client_addr.ss_family,
                  get_in_addr((struct sockaddr *)&client_addr), ipstr,
                  sizeof ipstr);
        std::cout << "server: got connection from " << ipstr << std::endl;
      } else {
        // Handle communication with an existing client
        handle_client(clients[i]);
        // close client fd
      }
    }
  }
  close(server_fd);
  close(epoll_fd);
  return 0;
}
