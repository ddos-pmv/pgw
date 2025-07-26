#include <arpa/inet.h>
#include <pgw_utils/config_loader.h>
#include <pgw_utils/imsi.h>
#include <sys/socket.h>

#include <iostream>
#include <memory>
#include <stdexcept>

#include "client_config.h"

int main(int argc, char* argv[]) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <IMSI>\n";
    std::exit(1);
  }

  // read imsi
  std::unique_ptr<protei::IMSI> imsi;
  std::vector<uint8_t> bcd;
  try {
    imsi = std::make_unique<protei::IMSI>(argv[1]);
    bcd = imsi->to_bcd();
  } catch (const std::exception& e) {
    std::cerr << "Invalid IMSI: " << e.what() << "\n";
  }

  // read config
  ClientConfig cfg;
  try {
    cfg = protei::load_config<ClientConfig>(
        "/home/userLinux/workspace/pgw/config/pgw_client.json");
  } catch (const std::exception& e) {
    std::cerr << "Invalid config: " << e.what() << "\n";
  }

  int sock = socket(AF_INET, SOCK_DGRAM, 0);
  if (sock < 0) {
    perror("socket");
    return 1;
  }
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(cfg.server_port);
  inet_pton(AF_INET, cfg.server_ip.data(), &addr.sin_addr);

  ssize_t sent =
      sendto(sock, bcd.data(), bcd.size(), 0, (sockaddr*)&addr, sizeof(addr));
  if (sent < 0) {
    perror("sendto");
  } else {
    std::cout << "Sent IMSI: " << imsi->str() << " (" << sent << " bytes BCD)"
              << std::endl;
  }

  close(sock);
  return 0;

  return 0;
}