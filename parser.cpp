


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
  std::cout << "bytes_received = " << bytes_received << std::endl;
  if (bytes_received <= 0 && this->remaining_from_last_request.length() == 0) {
    if (bytes_received == 0) {
      std::cout << "Client disconnected\n";
      this->connected = false;
      return false;
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return false;
      }
      // TODO: handle this case
      else if (errno == EINTR)
        return false;
      //Interrupted by signal, retry
      else {
        std::cerr << "Error receiving data from client: " << strerror(errno) << std::endl;
        throw ParsingError(INTERNAL_SERVER_ERROR, strerror(errno));
      }
    }
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
    std::cout << "creating new request\n";
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

Client::Client(int client_socket) : client_socket(client_socket), request(NULL), connected(true){
  response.clear();
  write_offset = 0;
  chunk = false;
  current_chunk.clear();
  chunk_offset = 0;
  final_chunk_sent = false;
  remaining_from_last_request.clear();
  last_time = std::time(NULL);
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

