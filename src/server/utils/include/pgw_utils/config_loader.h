#pragma once 

#include <nlohmann/json.hpp>

#include <string>
#include <fstream>

using json = nlohmann::json;
namespace protei
{
    struct ServerConfig
{
    uint16_t udp_port = 9000;
    uint16_t session_timeout_sec= 30;
    std::string cdr_file = "cdr.log";
    std::string log_level = "info";
    int thread_count = 1;
};

inline json load_config(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs) throw std::runtime_error("Failed to open config: " + path);

    json j;
    ifs >> j;
    return j;
}
}