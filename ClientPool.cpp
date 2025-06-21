#include "ClientPool.hpp"

ClientPool::ClientPool() {
  for (int i = 0; i < MAX; ++i) {
    freeList.push_back(i);
  }
}

Client *ClientPool::allocate(int fd, ServerConfig *server_conf) {
  if (freeList.empty())
    return 0;

  int idx = freeList.back();
  freeList.pop_back();

  return new (buffer + idx * sizeof(Client)) Client(fd, server_conf);
}

void ClientPool::deallocate(Client *obj) {
  if (!obj)
    return;

  obj->~Client();

  int idx = (reinterpret_cast<char *>(obj) - buffer) / sizeof(Client);

  if (!(idx >= 0 && idx < MAX)) {
    throw std::runtime_error("Invalid client");
  }

  freeList.push_back(idx);
}

Client *ClientPool::get(int idx) {
  if (idx < 0 || idx >= MAX)
    return 0;
  return reinterpret_cast<Client *>(buffer + idx * sizeof(Client));
}
