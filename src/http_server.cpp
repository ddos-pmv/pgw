#include "http_server.h"

#include <pgw_utils/logger_config.h>

namespace protei {

HttpServer::HttpServer(const HttpConfig& config,
                       std::shared_ptr<SessionManager> session_manager)
    : config_(config),
      session_manager_(std::move(session_manager)),
      server_(std::make_unique<httplib::Server>()),
      running_(false),
      shutdown_requested_(false) {
  setup_routes();

  // Настройка сервера
  server_->set_default_headers(
      {{"Server", "PGW-Server/1.0"}, {"Content-Type", "application/json"}});

  // Настройка таймаутов
  server_->set_read_timeout(30, 0);
  server_->set_write_timeout(30, 0);
  server_->set_idle_interval(1, 0);

  HTTP_LOG_INFO("HTTP Server initialized on port {}", config_.port);
}

HttpServer::~HttpServer() { stop(); }

void HttpServer::start() {
  if (running_.exchange(true)) {
    HTTP_LOG_WARN("HTTP Server already running");
    return;
  }

  server_thread_ = std::thread([this]() {
    HTTP_LOG_INFO("Starting HTTP Server on 0.0.0.0:{}", config_.port);

    if (!server_->listen("0.0.0.0", config_.port)) {
      HTTP_LOG_ERROR("Failed to start HTTP server on port {}", config_.port);
      running_ = false;
      return;
    }

    HTTP_LOG_INFO("HTTP Server stopped");
    running_ = false;
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  if (running_.load()) {
    HTTP_LOG_INFO("HTTP Server started successfully on port {}", config_.port);
  }
}

void HttpServer::stop() {
  if (!running_.exchange(false)) {
    return;
  }

  HTTP_LOG_INFO("Stopping HTTP Server...");

  if (server_) {
    server_->stop();
  }

  if (server_thread_.joinable()) {
    server_thread_.join();
  }

  HTTP_LOG_INFO("HTTP Server stopped");
}

void HttpServer::setup_routes() {
  server_->set_pre_routing_handler([](const httplib::Request& req,
                                      httplib::Response& res) {
    HTTP_LOG_DEBUG("HTTP {} {} from {}", req.method, req.path, req.remote_addr);
    return httplib::Server::HandlerResponse::Unhandled;
  });

  server_->Post("/check_subscriber",
                [this](const httplib::Request& req, httplib::Response& res) {
                  handle_check_subscriber(req, res);
                });

  server_->Post("/stop",
                [this](const httplib::Request& req, httplib::Response& res) {
                  handle_stop(req, res);
                });

  HTTP_LOG_INFO("HTTP routes configured: /check_subscriber, /stop");
}

void HttpServer::handle_check_subscriber(const httplib::Request& req,
                                         httplib::Response& res) {
  HTTP_LOG_TRACE("Processing /check_subscriber request");

  try {
    nlohmann::json json_req = nlohmann::json::parse(req.body);

    if (!json_req.contains("imsi") || !json_req["imsi"].is_string()) {
      res.set_content("Invalid request: missing 'imsi' field", "text/plain");
      res.status = 400;
      return;
    }

    std::string imsi = json_req["imsi"];
    bool is_active = session_manager_->session_exists(imsi);

    std::string response = is_active ? "active" : "not active";
    res.set_content(response, "text/plain");
    res.status = 200;

    HTTP_LOG_INFO("Checked subscriber {}: {}", imsi, response);

  } catch (const nlohmann::json::parse_error& e) {
    HTTP_LOG_ERROR("JSON parse error in /check_subscriber: {}", e.what());
    res.set_content("Invalid JSON", "text/plain");
    res.status = 400;

  } catch (const std::exception& e) {
    HTTP_LOG_ERROR("Error in /check_subscriber: {}", e.what());
    res.set_content("Internal server error", "text/plain");
    res.status = 500;
  }
}

void HttpServer::handle_stop(const httplib::Request& req,
                             httplib::Response& res) {
  HTTP_LOG_INFO("Processing /stop request - initiating graceful shutdown");

  try {
    // // Можно передать параметр в JSON
    // if (!req.body.empty()) {
    //   try {
    //     nlohmann::json json_req = nlohmann::json::parse(req.body);
    //     if (json_req.contains("rate") && json_req["rate"].is_number()) {
    //       shutdown_rate = json_req["rate"];
    //     }
    //   } catch (...) {
    //     // Игнорируем ошибки парсинга для /stop
    //   }
    // }

    res.set_content("Graceful shutdown initiated", "text/plain");
    res.status = 200;

    HTTP_LOG_INFO("Graceful shutdown requested via HTTP API ");

    // Запускаем graceful shutdown сессий
    session_manager_->start_graceful_shutdown();

    // Устанавливаем флаг для основного процесса
    shutdown_requested_ = true;

    std::thread([this]() {
      std::this_thread::sleep_for(std::chrono::milliseconds(500));
      stop();
    }).detach();

  } catch (const std::exception& e) {
    HTTP_LOG_ERROR("Error in /stop: {}", e.what());
    res.set_content("Internal server error", "text/plain");
    res.status = 500;
  }
}

// void HttpServer::handle_status(const httplib::Request& req,
//                                httplib::Response& res) {
//   HTTP_LOG_DEBUG("Processing /status request");

//   try {
//     nlohmann::json response = {
//         {"status", "running"},
//         {"http_port", config_.port},
//         // {"shutdown_requested", shutdown_requested_.load()},
//         {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
//                           std::chrono::system_clock::now().time_since_epoch())
//                           .count()}};

//     res.set_content(response.dump(2), "application/json");
//     res.status = 200;

//   } catch (const std::exception& e) {
//     HTTP_LOG_ERROR("Error in /status: {}", e.what());

//     nlohmann::json error_response = {{"error", "Internal Server Error"},
//                                      {"message", e.what()}};
//     res.set_content(error_response.dump(2), "application/json");
//     res.status = 500;
//   }
// }

// void HttpServer::handle_blacklist(const httplib::Request& req,
//                                   httplib::Response& res) {
//   HTTP_LOG_DEBUG("Processing /blacklist request");

//   try {
//     if (req.get_header_value("Content-Type").find("application/json") ==
//         std::string::npos) {
//       nlohmann::json error_response = {
//           {"error", "Invalid Content-Type"},
//           {"message", "Expected application/json"}};
//       res.set_content(error_response.dump(2), "application/json");
//       res.status = 400;
//       return;
//     }

//     nlohmann::json json_req = nlohmann::json::parse(req.body);

//     if (!json_req.contains("action") || !json_req.contains("imsi")) {
//       nlohmann::json error_response = {
//           {"error", "Invalid Request"},
//           {"message", "Missing 'action' or 'imsi' field"}};
//       res.set_content(error_response.dump(2), "application/json");
//       res.status = 400;
//       return;
//     }

//     std::string action = json_req["action"];
//     std::string imsi = json_req["imsi"];

//     if (action == "add") {
//       session_manager_->add_to_blacklist(imsi);
//       HTTP_LOG_INFO("Added IMSI {} to blacklist via HTTP", imsi);
//     } else if (action == "remove") {
//       session_manager_->remove_from_blacklist(imsi);
//       HTTP_LOG_INFO("Removed IMSI {} from blacklist via HTTP", imsi);
//     } else if (action == "check") {
//       bool is_blacklisted = session_manager_->is_blacklisted(imsi);
//       nlohmann::json response = {
//           {"imsi", imsi},
//           {"blacklisted", is_blacklisted},
//           {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
//                             std::chrono::system_clock::now().time_since_epoch())
//                             .count()}};
//       res.set_content(response.dump(2), "application/json");
//       res.status = 200;
//       return;
//     } else {
//       nlohmann::json error_response = {
//           {"error", "Invalid Action"},
//           {"message", "Action must be 'add', 'remove', or 'check'"}};
//       res.set_content(error_response.dump(2), "application/json");
//       res.status = 400;
//       return;
//     }

//     nlohmann::json response = {
//         {"message", "Blacklist updated successfully"},
//         {"action", action},
//         {"imsi", imsi},
//         {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
//                           std::chrono::system_clock::now().time_since_epoch())
//                           .count()}};

//     res.set_content(response.dump(2), "application/json");
//     res.status = 200;

//   } catch (const nlohmann::json::parse_error& e) {
//     HTTP_LOG_ERROR("JSON parse error in /blacklist: {}", e.what());

//     nlohmann::json error_response = {{"error", "JSON Parse Error"},
//                                      {"message", e.what()}};
//     res.set_content(error_response.dump(2), "application/json");
//     res.status = 400;

//   } catch (const std::exception& e) {
//     HTTP_LOG_ERROR("Error in /blacklist: {}", e.what());

//     nlohmann::json error_response = {{"error", "Internal Server Error"},
//                                      {"message", e.what()}};
//     res.set_content(error_response.dump(2), "application/json");
//     res.status = 500;
//   }
// }
}  // namespace protei