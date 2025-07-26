#include <arpa/inet.h>
#include <pgw_utils/config_loader.h>
#include <pgw_utils/imsi.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

#include "client_config.h"

class UdpClient {
  int sock;
  sockaddr_in server_addr;

 public:
  UdpClient(const ClientConfig& cfg) {
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) {
      throw std::runtime_error("socket creation failed");
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(cfg.server_port);
    if (inet_pton(AF_INET, cfg.server_ip.data(), &server_addr.sin_addr) <= 0) {
      close(sock);
      throw std::runtime_error("invalid server IP address");
    }
  }

  ~UdpClient() {
    if (sock >= 0) {
      close(sock);
    }
  }

  void send_imsi(const std::string& imsi_str) {
    try {
      protei::IMSI imsi(imsi_str);
      auto bcd = imsi.to_bcd();

      ssize_t sent = sendto(sock, bcd.data(), bcd.size(), 0,
                            (sockaddr*)&server_addr, sizeof(server_addr));
      if (sent < 0) {
        perror("sendto failed");
      } else {
        std::cout << "Sent IMSI: " << imsi.str() << " (" << sent
                  << " bytes BCD)" << std::endl;
      }
    } catch (const std::exception& e) {
      std::cerr << "IMSI processing error: " << e.what() << "\n";
    }
  }
};

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <IMSI> [interval_ms]\n";
    std::cerr
        << "  interval_ms - sending interval in milliseconds (optional)\n";
    return 1;
  }

  try {
    // Read config once
    ClientConfig cfg = protei::load_config<ClientConfig>(
        "/home/userLinux/workspace/pgw/config/pgw_client.json");

    // Create client once
    UdpClient client(cfg);
    const std::string imsi = argv[1];

    // Single shot mode
    if (argc == 2) {
      client.send_imsi(imsi);
      return 0;
    }

    // Periodic mode
    if (argc == 3) {
      const int interval_ms = std::stoi(argv[2]);
      if (interval_ms <= 0) {
        throw std::invalid_argument("Interval must be positive");
      }

      while (true) {
        client.send_imsi(imsi);
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_ms));
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  }

  return 0;
}