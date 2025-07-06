#include "ClientPool.hpp"
#include "errors.hpp"
#include "parser.hpp"
#include "webserv.hpp"
#include "helpers.hpp"

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

void print_request_log(HttpRequest *request) {
  if (!request || !request->head_parsed)
    return;

  std::string method;
  std::map<HTTP_METHOD, std::string>::const_iterator it =
      method_map.find(request->get_method());
  if (it != method_map.end()) {
    method = it->second;
  } else {
    method = "UNKNOWN";
  }
  LOG_STREAM(INFO, "Request: \"" << method << " "
                                 << request->get_path().get_path()
                                 << " HTTP/1.1\"");
}

bool handle_client(Client &client, uint32_t actions,
                   std::vector<ServerConfig> &servers_conf) {
  int status_code = 0;

  if (actions & EPOLLIN) {
    // Read data from client and process request, then prepare a response:
    try {
      // NOTE: 100 Continue && 101 Switching Protocols
      // TODO: if "Client disconnected" client should be freed

      while (client.parse_loop()) {
        // setup the server_conf if head is parsed
        if (!(client.get_request()->server_conf) &&
            client.get_request()->head_parsed) {
          print_request_log(client.get_request());
          client.get_request()->setup_serverconf(servers_conf, client.port);
          std::cout << "server_conf = " << client.get_request()->server_conf
                    << std::endl;
        }
      }

      // NOTE: requests from the same client has different client objects
      //       it should be fine tho
      if (!client.get_request()) { // NOTE: when client disconnect without
                                   // sending any data or when parsing stops at
                                   // request body but the endofstream signal is
                                   // not read yet
        // TODO: remove client ??
        return true;
      }


      if (client.connected && !client.get_request()->request_is_ready()) { // don't block
        return true;
      }

      client.get_request()->get_body_tmpfile().close();

      if (!client.connected)
        return false; // free the client

    } catch (ParsingError &e) {
      catch_setup_serverconf(&client, servers_conf);
      status_code = static_cast<PARSING_ERROR>(e.get_type());
      LOG_STREAM(INFO, "Error: " << e.what());
    } catch (std::exception &e) {
      // TODO: handle this case
      catch_setup_serverconf(&client, servers_conf);
      LOG_STREAM(ERROR, e.what());
      status_code = 500;
    }
    try {
      if (status_code)
        send_special_response(client, status_code);
      else
        process_request(client);
    } catch (std::exception &e) {
      LOG_STREAM(ERROR, "Generating response failed: " << e.what());
      send_special_response(client, 500);
    }
  }

  // if (client.get_request() && client.get_request()->request_is_ready()) {
  //   LOG(DEBUG, "done");
  // }
  if (actions & EPOLLOUT) {
    if (!handle_write(client))
      return false;
  }

  if (actions & (EPOLLHUP | EPOLLERR)) {
    LOG(ERROR, "Client disconnected or error");
    return false;
  }
  return true;
}

int get_server_fd(std::string port) {

  int status;
  struct addrinfo hints, *servinfo;

  // Initialize hints structure and set socket preferences
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  // Get address information for binding
  if ((status = getaddrinfo(NULL, port.c_str(), &hints, &servinfo)) != 0) {
    LOG_STREAM(ERROR, "getaddrinfo error on port :" << port << ": "
                                                    << gai_strerror(status));
    return -1;
  }

  // print_addrinfo(servinfo);

  struct addrinfo *p;
  int server_fd;
  int yes = 1;
  // Iterate through address info results to create and bind socket
  for (p = servinfo; p != NULL; p = p->ai_next) {
    server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (server_fd == -1) {
      LOG_STREAM(ERROR, "socket on port :" << port << ": " << strerror(errno));
      continue;
    }
    if (set_nonblocking(server_fd) == -1) {
      LOG_STREAM(ERROR, "fcntl on port :" << port << ": " << strerror(errno));
      close(server_fd);
      freeaddrinfo(servinfo);
      return -1;
    }
    // Allow port reuse to avoid "Address already in use" errors
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) ==
        -1) {
      LOG_STREAM(ERROR,
                 "setsockopt on port :" << port << ": " << strerror(errno));
      close(server_fd);
      freeaddrinfo(servinfo);
      return -1;
    }
    if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_fd);
      LOG_STREAM(ERROR, "bind on port :" << port << ": " << strerror(errno));
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo);

  if (p == NULL) {
    LOG_STREAM(ERROR, "server: failed to bind on port :" << port);
    close(server_fd);
    return -1;
  }

  if (listen(server_fd, SOMAXCONN)) {
    LOG_STREAM(ERROR, "server: failed to listen on port " << port << ": "
                                                          << strerror(errno));
    close(server_fd);
    return -1;
  }
  return server_fd;
}

ServerConfig *get_server_by_fd(std::vector<ServerConfig> &servers_conf,
                               int fd) {
  for (std::vector<ServerConfig>::iterator it = servers_conf.begin();
       it != servers_conf.end(); ++it) {
    if (it->getFd() == fd)
      return &(*it);
  }
  return NULL;
}

void server(std::vector<ServerConfig> &servers_conf, int epoll_fd,
            struct epoll_event *ev, ClientPool *pool,
            std::map<int, Client *> *fd_to_client,
            std::map<int, std::string> &fd_to_port) {
  int nfds;
  socklen_t addr_size;
  struct sockaddr_storage client_addr;
  struct epoll_event events[MAX_EVENTS];
  int client_fd;
  Client *client;
  char ipstr[INET6_ADDRSTRLEN];
  std::map<int, std::string>::iterator it;

  for (;;) {
    // Wait for events on monitored file descriptors
    nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
    if (nfds == -1) {
      LOG_STREAM(ERROR, "epoll_wait: " << strerror(errno));
      continue;
    }

    for (int i = 0; i < nfds; i++) {
      it = fd_to_port.find(events[i].data.fd);
      if (it != fd_to_port.end()) {
        addr_size = sizeof client_addr;
        client_fd = accept(events[i].data.fd, (struct sockaddr *)&client_addr,
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
        ev->events = EPOLLIN | EPOLLET | EPOLLOUT;
        ev->data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, ev) == -1) {
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
        client->port = it->second;
        (*fd_to_client)[client_fd] = client;

        inet_ntop(client_addr.ss_family,
                  get_in_addr((struct sockaddr *)&client_addr), ipstr,
                  sizeof ipstr);
        LOG_STREAM(INFO, "Got connection from: " << ipstr << " on port: "
                                                 << client->port);
      } else {
        // Handle communication with an existing clients
        int client_fd = events[i].data.fd;
        std::map<int, Client *>::iterator it = fd_to_client->find(client_fd);
        if (it != fd_to_client->end()) {
          client = it->second;
          if (!handle_client(*client, events[i].events, servers_conf)) {
            free_client(epoll_fd, client, fd_to_client, pool);
          }
        } else {
          LOG_STREAM(ERROR, "Unknown fd " << client_fd);
        }
      }
    }
  }
}

int start_server(std::vector<ServerConfig> &servers_conf) {
  int epoll_fd;
  struct epoll_event ev;
  std::map<int, std::string> fd_to_port;
  std::vector<std::string> ports;
  std::string port;

  // Create epoll instance for event-driven I/O
  epoll_fd = epoll_create(1);
  if (epoll_fd == -1) {
    LOG_STREAM(ERROR, "epoll_create: " << strerror(errno));
    return 1;
  }

  // NOTE: what if there is no server
  for (std::vector<ServerConfig>::iterator it = servers_conf.begin();
       it != servers_conf.end(); ++it) {
    port = int_to_string(it->getPort());
    if (find_in_vec(ports, port) == -1) {
      int server_fd = get_server_fd(port);
      if (server_fd == -1)
        continue;

      // Configure epoll to monitor server socket for incoming connections
      ev.events = EPOLLIN;
      ev.data.fd = server_fd;
      if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
        close(server_fd);
        continue;
      }

      LOG_STREAM(INFO, "Server is listening on " << port);

      fd_to_port[server_fd] = port;
      it->setFd(server_fd);
      ports.push_back(port);
    }
  }

  ClientPool *pool;
  std::map<int, Client *> *fd_to_client;
  try {
    pool = new ClientPool();
    fd_to_client = new std::map<int, Client *>;
  } catch (const std::bad_alloc &e) {
    LOG_STREAM(ERROR, "Memory allocation failed: " << e.what());
    close(epoll_fd);
    return 1;
  }

  server(servers_conf, epoll_fd, &ev, pool, fd_to_client, fd_to_port);

  delete pool;
  delete fd_to_client;
  close(epoll_fd);
  return 0;
}
