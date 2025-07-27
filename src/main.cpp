#include <pgw_utils/config_loader.h>
#include <pgw_utils/imsi.h>
#include <pgw_utils/logger_config.h>
#include <sys/socket.h>

#include <iostream>

#include "concurrentqueue.h"
#include "event_dispatcher.h"
#include "server_config.h"
#include "udp_server.h"

int main(int argc, char *argv[]) {
  try {
    // protei::ServerConfig config =
    // protei::load_config("/home/userLinux/workspace/pgw/config/pgw_server.json");

    protei::IMSI imsi("12345678");

    protei::UdpConfig udp_config = protei::load_config<protei::UdpConfig>(
        "/home/userLinux/workspace/pgw/config/pgw_server.json");
    protei::HttpConfig http_config = protei::load_config<protei::HttpConfig>(
        "/home/userLinux/workspace/pgw/config/pgw_server.json");

    protei::LoggerConfig::configure_logger(udp_config.log_file,
                                           udp_config.log_level);

    UDP_INIT_LOGGER(udp_config.log_file, udp_config.log_level);
    HTTP_INIT_LOGGER(http_config.log_level, http_config.log_level);

    moodycamel::ConcurrentQueue<protei::Event> queue;
    protei::UdpServer udp(udp_config.port, queue);
    protei::EventDispatcher dispatcher(10, queue);

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