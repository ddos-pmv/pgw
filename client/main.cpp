#include <pgw_utils/config_loader.h>

#include <atomic>
#include <csignal>
#include <iostream>
#include <numeric>

#include "client_config.h"
#include "udp_client.h"

namespace {
std::atomic<bool> shutdown_requested{false};

void signal_handler(int signal) {
  if (signal == SIGINT || signal == SIGTERM) {
    spdlog::info("Shutdown signal received, stopping client...");
    shutdown_requested = true;
  }
}
}  // namespace

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <IMSI> [interval_ms] [timeout_ms]\n";
    return 1;
  }

  std::signal(SIGINT, signal_handler);
  std::signal(SIGTERM, signal_handler);

  try {
    ClientConfig cfg = protei::load_config<ClientConfig>(
        "/home/sergey/workspace/pgw/config/pgw_client.json");

    protei::LoggerConfig::configure_logger(cfg.log_file, cfg.log_level,
                                           "client");

    const std::string imsi = argv[1];

    std::chrono::milliseconds timeout(cfg.response_timeout_ms);
    if (argc >= 4) {
      timeout = std::chrono::milliseconds(std::stoi(argv[3]));
    }

    UdpClient client(cfg, timeout);

    // Single shot mode
    if (argc == 2 || argc == 4) {
      auto response = client.send_imsi_with_response(imsi);

      switch (response.result) {
        case UdpClient::SendResult::SUCCESS:
          spdlog::info("SUCCESS: {} {} ms", response.response_text,
                       response.response_time.count());
          return 0;
        case UdpClient::SendResult::TIMEOUT:
          spdlog::info("TIMEOUT after {} ms", timeout.count());
          return 2;
        default:
          spdlog::error("ERROR: {}", response.response_text);
          return 1;
      }
    }

    // Periodic mode
    if (argc >= 3) {
      const int interval_ms = std::stoi(argv[2]);
      if (interval_ms <= 0) {
        throw std::invalid_argument("Interval must be positive");
      }

      spdlog::info("Periodic mode: interval={}ms, timeout={}ms", interval_ms,
                   timeout.count());

      size_t total = 0, success = 0;
      std::vector<uint32_t>
          response_times;  // Only store successful response times
      auto start_time = std::chrono::steady_clock::now();

      while (!shutdown_requested.load()) {
        auto response = client.send_imsi_with_response(imsi);
        total++;

        if (response.result == UdpClient::SendResult::SUCCESS) {
          success++;
          response_times.push_back(response.response_time.count());
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
      }

      // Calculate average response time
      double avg_response_time = 0.0;
      if (!response_times.empty()) {
        uint64_t sum =
            std::accumulate(response_times.begin(), response_times.end(), 0ULL);
        avg_response_time = static_cast<double>(sum) / response_times.size();
      }

      auto final_duration = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - start_time);

      spdlog::warn(
          "Client {} completed: duration={}s, total={}, success={}, "
          "avg_response={:.1f}ms",
          imsi, final_duration.count(), total, success, avg_response_time);
    }

  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    spdlog::error("Fatal: {}", e.what());
    return 1;
  }

  return 0;
}