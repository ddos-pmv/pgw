#pragma once

#include <httplib.h>

#include <atomic>
#include <memory>
#include <thread>

#include "server_config.h"
#include "session_manager.h"

namespace protei {

class HttpServer {
 public:
  HttpServer(const HttpConfig& config,
             std::shared_ptr<SessionManager> session_manager);

  ~HttpServer();

  void start();
  void stop();

  bool is_running() const { return running_.load(); }
  bool is_shutdown_requested() const { return shutdown_requested_.load(); }

 private:
  void setup_routes();
  void handle_check_subscriber(const httplib::Request& req,
                               httplib::Response& res);
  void handle_stop(const httplib::Request& req, httplib::Response& res);

  HttpConfig config_;
  std::shared_ptr<SessionManager> session_manager_;
  std::unique_ptr<httplib::Server> server_;
  std::thread server_thread_;
  std::atomic<bool> running_;
  std::atomic<bool> shutdown_requested_;
};

}  // namespace protei