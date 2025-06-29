


#include "parser.hpp"
#include "helpers.hpp"
#include "webserv.hpp"
#include "errors.hpp"



int Client::recv(void *buffer, size_t len) {
  return ::recv(this->client_socket, buffer, len, 0);
}


// true: continue parsing
// false: stop parsing
bool Client::parse_loop() {
  char buffer[1024];

  int bytes_received = this->recv(buffer, sizeof(buffer));
  if (bytes_received <= 0 && this->remaining_from_last_request.length() == 0) {
    if (bytes_received == 0) {
      std::cout << "Client disconnected\n";
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return !this->request->request_is_ready();
      }
      // TODO: handle this case
      // else if (errno == EINTR)
      //Interrupted by signal, retry
      else {
        std::cerr << "Error receiving data from client: " << strerror(errno) << std::endl;
      }
    }
    return false;
    // TODO: close client_socket
  }

  std::string recieved = "";
  if (bytes_received > 0)
    recieved += std::string(buffer, bytes_received);
  recieved  = this->remaining_from_last_request + recieved; // TODO: if remaining_from_last_request get too big, throw header field too large or somethin
  // std::cout << recieved << std::endl;
  

  bool should_continue;
  if (this->request) {
    should_continue = this->request->parse_raw(recieved);
  }
  else {
    //TODO: handle failed new
    this->request = new HttpRequest();
    should_continue = this->request->parse_raw(recieved);
  }
  this->remaining_from_last_request = recieved;

  return should_continue;
}

Client::~Client() {
  close(this->client_socket);
  response.clear();
  write_offset = 0;
  chunk = false;
  current_chunk.clear();
  chunk_offset = 0;
  final_chunk_sent = false;
  remaining_from_last_request.clear();
  delete this->request;
}

Client::Client(int client_socket) : client_socket(client_socket), request(NULL){
  response.clear();
  write_offset = 0;
  chunk = false;
  current_chunk.clear();
  chunk_offset = 0;
  final_chunk_sent = false;
  remaining_from_last_request.clear();
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

