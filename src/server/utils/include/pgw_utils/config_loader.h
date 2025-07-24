#pragma once

#include <fstream>
#include <nlohmann/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace protei {

using json = nlohmann::json;

struct UdpConfig {
  std::string ip = "0.0.0.0";
  uint16_t port = 9000;
  uint16_t session_timeout_sec = 30;
};

struct HttpConfig {
  uint16_t port = 8080;
};

inline void from_json(const nlohmann::json &j, UdpConfig &cfg) {
  j.at("udp_ip").get_to(cfg.ip);
  j.at("udp_port").get_to(cfg.port);
  j.at("session_timeout_sec").get_to(cfg.session_timeout_sec);
}

inline void from_json(const nlohmann::json &j, HttpConfig &cfg) {
  j.at("http_port").get_to(cfg.port);
}

template <typename T>
T load_config(const std::string &path) {
  std::ifstream ifs(path);
  if (!ifs) throw std::runtime_error("Failed to open config: " + path);

  json j;
  ifs >> j;

  return j.get<T>();
}

}  // namespace protei
