#include "pgw_server_app.h"

#include <pgw_utils/config_loader.h>
#include <pgw_utils/logger_config.h>
#include <spdlog/spdlog.h>

#include <nlohmann/json.hpp>

#include "cdr_writer.h"
#include "event_dispatcher.h"
#include "http_server.h"
#include "udp_server.h"

namespace protei {

PGWServerApp::PGWServerApp(const std::string& config_path)
    : config_path_(config_path) {
  load_configs();
  setup_logging();
  load_blacklist();
  init_components();
}

void PGWServerApp::run() {
  spdlog::info("=== PGW SERVER STARTING ===");
  start_servers();
  spdlog::info("Press ENTER to stop...");
  std::cin.get();
}

void PGWServerApp::load_configs() {
  udp_config_ = load_config<UdpConfig>(config_path_);
  http_config_ = load_config<HttpConfig>(config_path_);
  session_config_ = load_config<SessionConfig>(config_path_);
}

void PGWServerApp::setup_logging() {
  LoggerConfig::configure_logger(udp_config_.log_file, udp_config_.log_level);
  UDP_INIT_LOGGER(udp_config_.log_file, udp_config_.log_level);
  HTTP_INIT_LOGGER(http_config_.log_file, http_config_.log_level);
}

void PGWServerApp::load_blacklist() {
  try {
    auto full_config = load_config<nlohmann::json>(config_path_);
    if (full_config.contains("blacklist") &&
        full_config["blacklist"].is_array()) {
      for (const auto& imsi : full_config["blacklist"]) {
        if (imsi.is_string()) {
          blacklist_.insert(imsi.get<std::string>());
        }
      }
      spdlog::info("Loaded {} IMSI entries into blacklist", blacklist_.size());
    }
  } catch (const std::exception& e) {
    spdlog::warn("Could not load blacklist: {}", e.what());
  }
}

void PGWServerApp::init_components() {
  cdr_writer_ = std::make_shared<CDRWriter>(session_config_.cdr_file);
  session_manager_ = std::make_shared<SessionManager>(
      std::chrono::seconds(session_config_.session_timeout_sec),
      session_config_.shutdown_rate, blacklist_,
      create_cdr_callback(*cdr_writer_));

  auto queue = std::make_shared<moodycamel::ConcurrentQueue<Event>>();
  dispatcher_ = std::make_unique<EventDispatcher>(session_config_.threads_count,
                                                  *queue, session_manager_);
  udp_ = std::make_unique<UdpServer>(udp_config_.port, *queue);
  http_ = std::make_unique<HttpServer>(http_config_, session_manager_);

  udp_->set_event_notification_callback(
      [this]() { dispatcher_->notify_event_available(); });
}

void PGWServerApp::start_servers() {
  udp_->start();
  http_->start();
}

}  // namespace protei
