#include "../include/ClientPool.hpp"
#include "../include/errors.hpp"
#include "../include/helpers.hpp"
#include "../include/parser.hpp"
#include "../include/webserv.hpp"

void free_client(int epoll_fd, Client *client,
                 std::map<int, Client *> *fd_to_client, ClientPool *pool) {

  int fd = client->get_socket();
  LOG_STREAM(INFO, "Client: " << fd << " on port " << client->port
                              << " has been freed.");
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
  HttpRequest *req = NULL;

  if (actions & EPOLLIN) {
    // Read data from client and process request, then prepare a response:
    try {
      while (client.parse_loop()) {
        // setup the server_conf if head is parsed
        req = client.get_request();
        if (req && !(req->server_conf) && req->head_parsed) {
          print_request_log(req);
          req->setup_serverconf(servers_conf, client.port);
          std::cout << "server_conf = " << req->server_conf << std::endl;
        }
      }

      if (!client.connected)
        return false;

      req = client.get_request();
      if (!req) {
        return true;
      }

      if (client.connected && !req->request_is_ready()) { // don't block
        return true;
      }

      req->get_body_tmpfile().close();
    } catch (ParsingError &e) {
      catch_setup_serverconf(&client, servers_conf);
      status_code = static_cast<PARSING_ERROR>(e.get_type());
      LOG_STREAM(WARNING, e.what());
    } catch (std::exception &e) {
      catch_setup_serverconf(&client, servers_conf);
      LOG_STREAM(ERROR, e.what());
      status_code = 500;
    }
    try {
      if (status_code) {
        client.error_code = true;
        send_special_response(client, status_code);
      } else
        process_request(client);
    } catch (std::exception &e) {
      LOG_STREAM(ERROR, "Generating response failed: " << e.what());
      send_special_response(client, 500);
    }
  }

  if (actions & EPOLLOUT) {
    if (client.error_code ||
        (client.get_request() && client.get_request()->request_is_ready())) {
      if (!handle_write(client))
        return false;
    }
  }

  if (actions & (EPOLLHUP | EPOLLERR)) {
    LOG(WARNING, "Client disconnected or error");
    return false;
  }

  if (client.free_client)
    return false;
  return true;
}

int get_server_fd(const std::string &port, const std::string &ip) {
  int status;
  struct addrinfo hints, *servinfo;

  // Initialize hints structure and set socket preferences
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;     // IPv4 or IPv6
  hints.ai_socktype = SOCK_STREAM; // TCP
  hints.ai_flags = AI_PASSIVE; // For wildcard IP (if ip is NULL or "0.0.0.0")

  // Get address information for binding
  if ((status = getaddrinfo(ip.c_str(), port.c_str(), &hints, &servinfo)) !=
      0) {
    LOG_STREAM(ERROR, "getaddrinfo error on " << ip << ":" << port << ": "
                                              << gai_strerror(status));
    return -1;
  }

  struct addrinfo *p;
  int server_fd = -1;
  int yes = 1;
  // Iterate through address info results to create and bind socket
  for (p = servinfo; p != NULL; p = p->ai_next) {
    server_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
    if (server_fd == -1) {
      LOG_STREAM(ERROR,
                 "socket on " << ip << ":" << port << ": " << strerror(errno));
      continue;
    }
    if (set_nonblocking(server_fd) == -1) {
      LOG_STREAM(ERROR,
                 "fcntl on " << ip << ":" << port << ": " << strerror(errno));
      close(server_fd);
      freeaddrinfo(servinfo);
      return -1;
    }
    // Allow port reuse to avoid "Address already in use" errors
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(int)) ==
        -1) {
      LOG_STREAM(ERROR, "setsockopt on " << ip << ":" << port << ": "
                                         << strerror(errno));
      close(server_fd);
      freeaddrinfo(servinfo);
      return -1;
    }
    if (bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
      close(server_fd);
      LOG_STREAM(ERROR,
                 "bind on " << ip << ":" << port << ": " << strerror(errno));
      continue;
    }
    break;
  }

  freeaddrinfo(servinfo);

  if (p == NULL) {
    LOG_STREAM(ERROR, "server: failed to bind on " << ip << ":" << port);
    if (server_fd != -1)
      close(server_fd);
    return -1;
  }

  if (listen(server_fd, SOMAXCONN) == -1) {
    LOG_STREAM(ERROR, "server: failed to listen on "
                          << ip << ":" << port << ": " << strerror(errno));
    close(server_fd);
    return -1;
  }

  return server_fd;
}

ServerConfig *get_server_by_fd(std::vector<ServerConfig> &servers_conf,
                               int fd) {
  for (std::vector<ServerConfig>::iterator it = servers_conf.begin();
       it != servers_conf.end(); ++it) {
    std::vector<int> vec = it->getFds();
    for (std::vector<int>::iterator it2 = vec.begin(); it2 != vec.end();
         ++it2) {
      if (*it2 == fd) {
        return &(*it);
      }
    }
  }
  return NULL;
}

void free_unused_clients(int epoll_fd, std::map<int, Client *> *fd_to_client,
                         ClientPool *pool) {
  std::map<int, Client *>::iterator it = fd_to_client->begin();
  std::map<int, Client *>::iterator end = fd_to_client->end();
  while (it != end) {
    std::time_t current_time = std::time(NULL);
    double elapsed = std::difftime(current_time, it->second->last_time);
    if (elapsed >= CLIENT_TIMEOUT) {
      LOG_STREAM(INFO, "Timeout: " << elapsed);
      free_client(epoll_fd, it->second, fd_to_client, pool);
      it = fd_to_client->begin();
      end = fd_to_client->end();
    } else
      it++;
  }
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
  bool result;

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

        ev->events = EPOLLIN;
        ev->data.fd = client_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, ev) == -1) {
          LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
          close(client_fd);
          continue;
        }

        free_unused_clients(epoll_fd, fd_to_client, pool);

        client = pool->allocate(client_fd);
        if (!client) {
          LOG_STREAM(ERROR, "No free client slots available");
          if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client_fd, NULL) == -1)
            LOG_STREAM(WARNING, "epoll_ctl: " << strerror(errno));
          close(client_fd);
          continue;
        }
        client->port = it->second;
        (*fd_to_client)[client_fd] = client;

        const char *ptr = inet_ntop(
            client_addr.ss_family, get_in_addr((struct sockaddr *)&client_addr),
            ipstr, sizeof ipstr);
        if (!ptr)
          LOG_STREAM(WARNING, "inet_ntop: " << strerror(errno));
        else {
          client->addr = std::string(ipstr);
          LOG_STREAM(INFO, "Got connection from: " << ipstr << " on port: "
                                                   << client->port);
        }
      } else {
        // Handle communication with an existing clients
        int client_fd = events[i].data.fd;
        std::map<int, Client *>::iterator it = fd_to_client->find(client_fd);
        if (it != fd_to_client->end()) {
          client = it->second;
          client->last_time = std::time(NULL);
          result = handle_client(*client, events[i].events, servers_conf);
          if (!result) {
            free_client(epoll_fd, client, fd_to_client, pool);
          } else {
            if (client->connected &&
                ((client->get_request() &&
                  client->get_request()->request_is_ready()) ||
                 client->error_code)) {
              ev->events = EPOLLIN | EPOLLOUT;
              ev->data.fd = client_fd;
              if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->get_socket(), ev))
                LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
            } else if (!client->get_request()) {
              ev->events = EPOLLIN;
              ev->data.fd = client_fd;
              if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, client->get_socket(), ev))
                LOG_STREAM(ERROR, "epoll_ctl: " << strerror(errno));
            }
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
  std::string ip;

  // Create epoll instance for event-driven I/O
  epoll_fd = epoll_create(1);
  if (epoll_fd == -1) {
    LOG_STREAM(ERROR, "epoll_create: " << strerror(errno));
    return 1;
  }

  for (std::vector<ServerConfig>::iterator it = servers_conf.begin();
       it != servers_conf.end(); ++it) {
    std::map<std::string, int> inter_ports = it->getInterPort();
    for (std::map<std::string, int>::iterator it2 = inter_ports.begin();
         it2 != inter_ports.end(); ++it2) {
      port = int_to_string(it2->second);
      ip = it2->first;
      int server_fd = get_server_fd(port, ip);
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

      if (it->getServerNames().empty())
        LOG_STREAM(INFO, "Listening on " << it2->first << ":" << port);
      else
        LOG_STREAM(INFO, "Listening on " << it2->first << ":" << port << " - " << it->getServerNames()[0]);

      fd_to_port[server_fd] = port;
      it->addFd(server_fd);
      ports.push_back(port);
    }
  }

  //TODO: if post/post not allowd can the request be stoped before uploading the body
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

  LOG(INFO, "Server started");
  server(servers_conf, epoll_fd, &ev, pool, fd_to_client, fd_to_port);

  for (std::map<int, Client *>::iterator it = fd_to_client->begin(); it != fd_to_client->end(); ++it) {
    it->second->~Client();
  }
  delete pool;
  delete fd_to_client;
  close(epoll_fd);
  return 0;
}
