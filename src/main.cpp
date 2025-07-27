#include <pgw_utils/config_loader.h>
#include <pgw_utils/imsi.h>
#include <pgw_utils/logger_config.h>
#include <sys/socket.h>

#include <iostream>

#include "concurrentqueue.h"
#include "event_dispatcher.h"
#include "server_config.h"
#include "session_manager.h"
#include "udp_server.h"

int main(int argc, char* argv[]) {
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
    HTTP_INIT_LOGGER(http_config.log_file, http_config.log_level);

    spdlog::info("=== PGW SERVER STARTING ===");

    // Load black list
    std::unordered_set<std::string> blacklist;
    try {
      nlohmann::json full_config = protei::load_config<nlohmann::json>(
          "/home/userLinux/workspace/pgw/config/pgw_server.json");

      if (full_config.contains("blacklist") &&
          full_config["blacklist"].is_array()) {
        for (const auto& imsi : full_config["blacklist"]) {
          if (imsi.is_string()) {
            blacklist.insert(imsi.get<std::string>());
          }
        }

        spdlog::info("Loaded {} IMSI entries into blacklist", blacklist.size());
      }
    } catch (const std::exception& e) {
      spdlog::warn("Could not load blacklist from config: {}", e.what());
    }

    // Init Session Manager with CDR callback
    auto session_manager = std::make_shared<protei::SessionManager>(
        std::chrono::seconds(udp_config.session_timeout_sec), blacklist);

    // Init event queue and components
    moodycamel::ConcurrentQueue<protei::Event> queue;
    protei::UdpServer udp(udp_config.port, queue);
    protei::EventDispatcher dispatcher(10, queue, session_manager);

    // Startring servers
    udp.start();

    int a;
    std::cout << "Press any key to stop" << std::endl;
    std::cin >> a;

    // protei::Server udp(udp_config);
  } catch (const std::exception& e) {
    std::cerr << e.what();
  }

  // server.start();

  return 0;
}