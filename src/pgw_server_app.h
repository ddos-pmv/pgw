#pragma once

#include <string>
#include <unordered_set>

#include "cdr_writer.h"
#include "event_dispatcher.h"
#include "http_server.h"
#include "server_config.h"
#include "session_manager.h"
#include "udp_server.h"

namespace protei {
class PGWServerApp {
 public:
  explicit PGWServerApp(const std::string& config_paht);
  void run();

 private:
  void load_configs();
  void setup_logging();
  void load_blacklist();
  void init_components();
  void start_servers();

  std::string config_path_;
  std::unordered_set<std::string> blacklist_;

  UdpConfig udp_config_;
  HttpConfig http_config_;
  SessionConfig session_config_;

  std::shared_ptr<CDRWriter> cdr_writer_;
  std::shared_ptr<SessionManager> session_manager_;
  std::unique_ptr<UdpServer> udp_;
  std::unique_ptr<HttpServer> http_;
  std::unique_ptr<EventDispatcher> dispatcher_;
};
}  // namespace protei