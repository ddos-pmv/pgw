#pragma once
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>

#include "client_config.h"

class UdpClient {
  int sock;
  sockaddr_in server_addr;
  std::chrono::milliseconds response_timeout_;

 public:
  UdpClient(const ClientConfig& cfg, std::chrono::milliseconds timeout =
                                         std::chrono::milliseconds(5000));

  ~UdpClient();

  enum class SendResult {
    SUCCESS,
    SEND_FAILED,
    TIMEOUT,
    INVALID_RESPONSE,
    NETWORK_ERROR
  };

  struct Response {
    SendResult result;
    std::string response_text;
    std::chrono::milliseconds response_time;
  };

  Response send_imsi_with_response(const std::string& imsi_str);

  // Legacy method for backward compatibility
  void send_imsi(const std::string& imsi_str);

 private:
  Response wait_for_response();

  Response receive_response();
};