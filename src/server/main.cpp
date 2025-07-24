#include <pgw_utils/config_loader.h>
#include <sys/socket.h>

#include <iostream>

#include "udp_server.h"

int main(int argc, char *argv[]) {
  try {
    // protei::ServerConfig config =
    // protei::load_config("/home/userLinux/workspace/pgw/config/pgw_server.json");

    protei::UdpConfig udp_config = protei::load_config<protei::UdpConfig>(
        "/home/userLinux/workspace/pgw/config/pgw_server.json");

    std::cout << udp_config.port << std::endl;

    // protei::Server udp(udp_config);
  } catch (...) {
  }

  // server.start();

  return 0;
}