#ifndef CLIENTPOOL_HPP
#define CLIENTPOOL_HPP

#include "parser.hpp"

#define MAX_CLIENTS 500

class ClientPool {
private:
  enum { MAX = MAX_CLIENTS };
  char buffer[MAX * sizeof(Client)];
  std::vector<int> freeList;

public:
  ClientPool();

  Client *allocate(int fd);
  void deallocate(Client *obj);
  Client *get(int idx);
};

#endif
