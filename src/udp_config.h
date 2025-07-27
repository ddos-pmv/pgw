// server/udp_config.h
#pragma once

#include <nlohmann/json.hpp>
#include <string>
namespace protei {
struct UdpConfig {
  std::string ip = "0.0.0.0";
  uint16_t port = 9000;
  uint16_t session_timeout_sec = 30;
  std::string log_level = "INFO";
  std::string log_file = "udp.log";
};

inline void from_json(const nlohmann::json& j, UdpConfig& cfg) {
  j.at("udp_ip").get_to(cfg.ip);
  j.at("udp_port").get_to(cfg.port);
  j.at("session_timeout_sec").get_to(cfg.session_timeout_sec);
  j.at("log_level").get_to(cfg.log_level);
  j.at("log_file").get_to(cfg.log_file);
}
}  // namespace protei