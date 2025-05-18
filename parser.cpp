


#include "parser.hpp"
#include "helpers.hpp"
#include "webserv.hpp"



int Client::recv(void *buffer, size_t len) {
  return ::recv(this->client_socket, buffer, len, 0);
}


bool Client::parse_loop() {
  char buffer[1024];

  int bytes_received = this->recv(buffer, sizeof(buffer));
  if (bytes_received <= 0) {
    if (bytes_received == 0) {
      std::cout << "Client disconnected\n";
    } else {
      if (errno == EAGAIN) {
        return true;
      }
      else {
        std::cerr << "Error receiving data from client\n";
      }
    }
    return false;
    // TODO: close client_socket
  }
  std::string recieved = std::string(buffer, bytes_received);
  recieved  = this->remaining_from_last_request + recieved; // TODO: if remaining_from_last_request get too big, throw header field too large or somethin
  std::cout << recieved << std::endl;
  if (this->request)
    this->request->parse_raw(recieved);
  else {
    this->request = new HttpRequest();
    this->request->parse_raw(recieved);
  }
  this->remaining_from_last_request = recieved;
  this->request->print();

  return true;
}

Client::~Client() {
  close(this->client_socket);
  delete this->request;
}

Client::Client() : client_socket(0), request(NULL){

}

Client::Client(int client_socket) : client_socket(client_socket), request(NULL){

}

Client & Client::operator = (const Client &client) {
  if (&client != this) {
    this->client_socket = client.client_socket;
    this->remaining_from_last_request = client.remaining_from_last_request;
    if (client.request)
      this->request = client.request->clone();
    else
      this->request = NULL;
  }
  return *this;
}

