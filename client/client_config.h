#pragma once

#include <nlohmann/json.hpp>
#include <string>

struct ClientConfig {
  std::string server_ip = "127.0.0.1";
  uint16_t server_port = 9000;
  std::string log_file = "client.log";
  std::string log_level = "INFO";
  size_t response_timeout_ms = 5000;
};

inline void from_json(const nlohmann::json& j, ClientConfig& cfg) {
  j.at("server_ip").get_to(cfg.server_ip);
  j.at("server_port").get_to(cfg.server_port);
  j.at("log_level").get_to(cfg.log_level);
  j.at("log_file").get_to(cfg.log_file);
  j.at("response_timeout_ms").get_to(cfg.response_timeout_ms);
}