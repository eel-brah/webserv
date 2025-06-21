#include "webserv.hpp"

int main(int ac, char *av[]) {
  if (ac != 2) {
    LOG(ERROR, "Invalid number of arguments");
    LOG_STREAM(INFO, "Usage: " << av[0] << " [Config_file]");
    return 1;
  }
  LOG(INFO, "Server started");
  std::vector<ServerConfig> servers_conf;
  try {
    servers_conf = parseConfig(av[1]);
  } catch (std::exception &e) {
    LOG_STREAM(ERROR, "Config file parsing failed: " << e.what());
    return 1;
  }
  return start_server(servers_conf);
}
