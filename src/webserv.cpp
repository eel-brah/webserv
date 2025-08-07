#include "../include/webserv.hpp"

int main(int ac, char *av[]) {
  std::string conf_file = CONF_FILE;
  if (ac != 2 && ac != 1) {
    LOG(ERROR, "Invalid number of arguments");
    LOG_STREAM(INFO, "Usage: " << av[0] << " [Config_file]");
    return 1;
  } else if (ac == 2)
    conf_file = av[1];
  LOG(INFO, "Server starting...");
  std::vector<ServerConfig> servers_conf;
  try {
    servers_conf = parseConfig(conf_file);
  } catch (std::exception &e) {
    LOG_STREAM(ERROR, "Config file parsing failed: " << e.what());
    return 1;
  }
  return start_server(servers_conf);
}
