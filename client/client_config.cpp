#include "client_config.h"

void from_json(const nlohmann::json& j, ClientConfig& cfg) {
  j.at("server_ip").get_to(cfg.server_ip);
  j.at("server_port").get_to(cfg.server_port);
  j.at("log_level").get_to(cfg.log_level);
  j.at("log_file").get_to(cfg.log_file);
  j.at("response_timeout_ms").get_to(cfg.response_timeout_ms);
}