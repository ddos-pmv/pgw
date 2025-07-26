#include <pgw_utils/config_loader.h>
#include <pgw_utils/imsi.h>
#include <sys/socket.h>

#include <iostream>

#include "concurrentqueue.h"
#include "event_dispatcher.h"
#include "udp_config.h"
#include "udp_server.h"

int main(int argc, char *argv[]) {
  try {
    // protei::ServerConfig config =
    // protei::load_config("/home/userLinux/workspace/pgw/config/pgw_server.json");

    protei::IMSI imsi("12345678");

    protei::UdpConfig udp_config = protei::load_config<protei::UdpConfig>(
        "/home/userLinux/workspace/pgw/config/pgw_server.json");

    moodycamel::ConcurrentQueue<protei::Event> queue;
    protei::UdpServer udp(udp_config.port, queue);
    protei::EventDisptacher dispatcher(10, queue);

    udp.start();

    int a;
    std::cout << "Press any key to stop" << std::endl;
    std::cin >> a;

    // protei::Server udp(udp_config);
  } catch (const std::exception &e) {
    std::cerr << e.what();
  }

  // server.start();

  return 0;
}