#include <pgw_utils/config_loader.h>
#include <pgw_utils/imsi.h>
#include <pgw_utils/logger_config.h>
#include <sys/socket.h>

#include <iostream>

#include "cdr_writer.h"
#include "concurrentqueue.h"
#include "event_dispatcher.h"
#include "http_server.h"
#include "server_config.h"
#include "session_manager.h"
#include "udp_server.h"

namespace {
std::atomic<bool> shutdown_requested{false};

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    spdlog::info("Shutdown signal received, stopping servers...");
    shutdown_requested = true;
  }
}
}  // namespace

int main(int argc, char* argv[]) {
  // if (argc < 2) {
  //   std::cerr << "Usage: " << argv[0] << "<path_to_config.json>\n";
  //   return 1;
  // }

  // std::string path_to_config = argv[1];

  std::string path_to_config =
      "/home/userLinux/workspace/pgw/config/pgw_server.json";

  try {
    // protei::ServerConfig config =
    // protei::load_config("/home/userLinux/workspace/pgw/config/pgw_server.json");

    // Init server's config
    protei::UdpConfig udp_config =
        protei::load_config<protei::UdpConfig>(path_to_config);
    protei::HttpConfig http_config =
        protei::load_config<protei::HttpConfig>(path_to_config);
    protei::SessionConfig session_config =
        protei::load_config<protei::SessionConfig>(path_to_config);

    protei::LoggerConfig::configure_logger(udp_config.log_file,
                                           udp_config.log_level);

    UDP_INIT_LOGGER(udp_config.log_file, udp_config.log_level);
    HTTP_INIT_LOGGER(http_config.log_file, http_config.log_level);

    spdlog::info("=== PGW SERVER STARTING ===");

    // Load black list
    std::unordered_set<std::string> blacklist;
    try {
      nlohmann::json full_config =
          protei::load_config<nlohmann::json>(path_to_config);

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
    auto cdr_writer =
        std::make_shared<protei::CDRWriter>(session_config.cdr_file);
    auto session_manager = std::make_shared<protei::SessionManager>(
        std::chrono::seconds(session_config.session_timeout_sec),
        session_config.shutdown_rate, blacklist,
        protei::create_cdr_callback(*cdr_writer));

    // Init event queue and components
    moodycamel::ConcurrentQueue<protei::Event> queue;
    protei::UdpServer udp(udp_config.port, queue);
    protei::HttpServer http(http_config, session_manager);
    protei::EventDispatcher dispatcher(session_config.threads_count, queue,
                                       session_manager);

    // Startring servers
    udp.start();
    http.start();

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