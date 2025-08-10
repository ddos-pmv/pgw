#pragma once

#include <nlohmann/json.hpp>

struct ClientConfig {
  std::string server_ip = "127.0.0.1";
  uint16_t server_port = 9000;
  std::string log_file = "client.log";
  std::string log_level = "INFO";
  size_t response_timeout_ms = 5000;
};

void from_json(const nlohmann::json& j, ClientConfig& cfg);