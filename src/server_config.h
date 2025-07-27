#pragma once

#include <nlohmann/json.hpp>
#include <string>
namespace protei {

struct UdpConfig {
  std::string ip = "0.0.0.0";
  uint16_t port = 9000;
  std::string log_level = "INFO";
  std::string log_file = "udp.log";
};
struct HttpConfig {
  uint16_t port = 8080;
  std::string log_level = "INFO";
  std::string log_file = "udp.log";
};

struct SessionConfig {
  uint16_t session_timeout_sec = 30;
  std::string cdr_file = "cdr.log";
  uint16_t threads_count = 10;
  size_t shutdown_rate = 10;
};

inline void from_json(const nlohmann::json& j, UdpConfig& cfg) {
  j.at("udp_ip").get_to(cfg.ip);
  j.at("udp_port").get_to(cfg.port);

  j.at("udp_log_level").get_to(cfg.log_level);
  j.at("udp_log_file").get_to(cfg.log_file);
}

inline void from_json(const nlohmann::json& j, HttpConfig& cfg) {
  j.at("http_port").get_to(cfg.port);
  j.at("http_log_level").get_to(cfg.log_level);
  j.at("http_log_file").get_to(cfg.log_file);
}

inline void from_json(const nlohmann::json& j, SessionConfig& cfg) {
  j.at("session_timeout_sec").get_to(cfg.session_timeout_sec);
  j.at("cdr_file").get_to(cfg.cdr_file);
  j.at("threads_count").get_to(cfg.threads_count);
  j.at("shutdown_rate").get_to(cfg.shutdown_rate);
}

}  // namespace protei