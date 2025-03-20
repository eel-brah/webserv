#include "parser.hpp"

#define MAX_EVENTS 10

// Function to set a socket to non-blocking mode
int set_non_blocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Error getting flags for socket\n";
        return -1;
    }

    flags |= O_NONBLOCK;
    flags |= SO_REUSEADDR;
    if (fcntl(sockfd, F_SETFL, flags) == -1) {
        std::cerr << "Error setting socket to non-blocking\n";
        return -1;
    }
    const int enable = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        std::cerr << "Error setting socket to non-blocking\n";
        return -1;
    }
    return 0;
}

// Function to handle the communication with each client
void handle_client(Client client) {
    //char buffer[1024];
    
    // Receive data from the client
    //ssize_t bytes_received = client.recv(buffer, sizeof(buffer));
    //std::cout << bytes_received << std::endl;
    while (client.parse_loop()) {
    }
}

void start_server(const std::string& host, int port) {
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Error creating socket\n";
        return;
    }

    // Set the socket to non-blocking
    if (set_non_blocking(server_socket) == -1) {
        close(server_socket);
        return;
    }

    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(host.c_str());
    server_addr.sin_port = htons(port);

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error binding socket\n";
        close(server_socket);
        return;
    }

    // Start listening for incoming connections
    if (listen(server_socket, 5) < 0) {
        std::cerr << "Error listening on socket\n";
        close(server_socket);
        return;
    }

    std::cout << "Server started on " << host << ":" << port << "\n";

    // Create epoll instance
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        std::cerr << "Error creating epoll instance\n";
        close(server_socket);
        return;
    }

    // Add the server socket to epoll
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = server_socket;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_socket, &event) == -1) {
        std::cerr << "Error adding server socket to epoll\n";
        close(epoll_fd);
        close(server_socket);
        return;
    }

    struct epoll_event events[MAX_EVENTS];
    Client clients[MAX_EVENTS];

    while (true) {
        int num_events = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (num_events == -1) {
            std::cerr << "Error in epoll_wait\n";
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == server_socket) {
                // New incoming connection
                sockaddr_in client_addr;
                socklen_t client_addr_len = sizeof(client_addr);
                int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);
                if (client_socket == -1) {
                    std::cerr << "Error accepting client connection\n";
                    continue;
                }

                // Set client socket to non-blocking
                if (set_non_blocking(client_socket) == -1) {
                    close(client_socket);
                    continue;
                }

                // Add the new client socket to epoll
                event.events = EPOLLIN | EPOLLET;  // Edge-triggered event for non-blocking IO
                event.data.fd = client_socket;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_socket, &event) == -1) {
                    std::cerr << "Error adding client socket to epoll\n";
                    close(client_socket);
                    continue;
                }

                new (&clients[i]) Client(client_socket);
                std::cout << "New client connected\n";
            } else {
                // Handle communication with an existing client
                handle_client(clients[i]);
            }
        }
    }

    close(epoll_fd);
    close(server_socket);
}

int main() {
    std::string host = "127.0.0.1";
    int port = 9999;
    start_server(host, port);
    return 0;
}

