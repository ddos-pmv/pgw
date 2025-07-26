#include <pgw_utils/config_loader.h>
#include <pgw_utils/imsi.h>
#include <sys/socket.h>

#include <iostream>

#include "udp_server.h"

int main(int argc, char *argv[]) {
  try {
    // protei::ServerConfig config =
    // protei::load_config("/home/userLinux/workspace/pgw/config/pgw_server.json");

    protei::IMSI<> imsi("12345678");

    protei::UdpConfig udp_config = protei::load_config<protei::UdpConfig>(
        "/home/userLinux/workspace/pgw/config/pgw_server.json");

    std::cout << udp_config.port << std::endl;

    // protei::Server udp(udp_config);
  } catch (...) {
    std::cerr << "exception" << std::endl;
  }

  // server.start();

  return 0;
}