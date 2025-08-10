#include "udp_client.h"

#include <pgw_utils/imsi.h>
#include <pgw_utils/logger_config.h>
#include <unistd.h>

UdpClient::UdpClient(const ClientConfig& cfg, std::chrono::milliseconds timeout)
    : response_timeout_(timeout) {
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

  // Set socket to non-blocking for timeout support
  int flags = fcntl(sock, F_GETFL, 0);
  if (flags < 0) {
    close(sock);
    throw std::runtime_error("failed to get socket flags");
  }

  if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) {
    close(sock);
    throw std::runtime_error("failed to set socket non-blocking");
  }

  spdlog::info("UDP Client initialized - Server: {}:{}, Timeout: {}ms",
               cfg.server_ip, cfg.server_port, timeout.count());
}

UdpClient::~UdpClient() {
  if (sock >= 0) {
    close(sock);
    spdlog::debug("UDP Client socket closed");
  }
}

UdpClient::Response UdpClient::send_imsi_with_response(
    const std::string& imsi_str) {
  auto start_time = std::chrono::steady_clock::now();

  try {
    protei::IMSI imsi(imsi_str);
    auto bcd = imsi.to_bcd();

    spdlog::debug("Sending IMSI: {} ({} bytes BCD)", imsi.str(), bcd.size());

    // Send request
    ssize_t sent = sendto(sock, bcd.data(), bcd.size(), 0,
                          (sockaddr*)&server_addr, sizeof(server_addr));
    if (sent < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        spdlog::error("Send would block - this shouldn't happen for UDP");
        return {SendResult::SEND_FAILED, "", std::chrono::milliseconds(0)};
      }
      spdlog::error("sendto failed: {}", strerror(errno));
      return {SendResult::SEND_FAILED, strerror(errno),
              std::chrono::milliseconds(0)};
    }

    if (static_cast<size_t>(sent) != bcd.size()) {
      spdlog::warn("Partial send: {}/{} bytes", sent, bcd.size());
    }

    spdlog::trace("Successfully sent {} bytes", sent);

    // Wait for response with timeout
    auto response = wait_for_response();

    auto end_time = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);

    response.response_time = duration;

    if (response.result == SendResult::SUCCESS) {
      spdlog::info("IMSI: {} -> Response: '{}' ({}ms)", imsi.str(),
                   response.response_text, duration.count());
    } else {
      spdlog::warn("IMSI: {} -> Failed: {} ({}ms)", imsi.str(),
                   static_cast<int>(response.result), duration.count());
    }

    return response;

  } catch (const std::exception& e) {
    spdlog::error("IMSI processing error: {}", e.what());
    return {SendResult::INVALID_RESPONSE, e.what(),
            std::chrono::milliseconds(0)};
  }
}

// Legacy method for backward compatibility
void UdpClient::send_imsi(const std::string& imsi_str) {
  auto response = send_imsi_with_response(imsi_str);

  switch (response.result) {
    case SendResult::SUCCESS:
      std::cout << "Sent IMSI: " << imsi_str << " -> Response: '"
                << response.response_text << "' ("
                << response.response_time.count() << "ms)" << std::endl;
      break;
    case SendResult::TIMEOUT:
      std::cout << "Sent IMSI: " << imsi_str << " -> TIMEOUT after "
                << response_timeout_.count() << "ms" << std::endl;
      break;
    case SendResult::SEND_FAILED:
      std::cout << "Failed to send IMSI: " << imsi_str << " -> "
                << response.response_text << std::endl;
      break;
    default:
      std::cout << "Error processing IMSI: " << imsi_str << " -> "
                << response.response_text << std::endl;
      break;
  }
}

UdpClient::Response UdpClient::wait_for_response() {
  fd_set read_fds;
  struct timeval timeout;

  // Convert timeout to timeval
  timeout.tv_sec = response_timeout_.count() / 1000;
  timeout.tv_usec = (response_timeout_.count() % 1000) * 1000;

  FD_ZERO(&read_fds);
  FD_SET(sock, &read_fds);

  int ready = select(sock + 1, &read_fds, nullptr, nullptr, &timeout);

  if (ready < 0) {
    if (errno == EINTR) {
      spdlog::debug("select interrupted by signal");
      return {SendResult::NETWORK_ERROR, "Interrupted",
              std::chrono::milliseconds(0)};
    }
    spdlog::error("select failed: {}", strerror(errno));
    return {SendResult::NETWORK_ERROR, strerror(errno),
            std::chrono::milliseconds(0)};
  }

  if (ready == 0) {
    spdlog::debug("Response timeout after {}ms", response_timeout_.count());
    return {SendResult::TIMEOUT, "No response within timeout",
            std::chrono::milliseconds(0)};
  }

  if (FD_ISSET(sock, &read_fds)) {
    return receive_response();
  }

  return {SendResult::NETWORK_ERROR, "Unexpected select result",
          std::chrono::milliseconds(0)};
}

UdpClient::Response UdpClient::receive_response() {
  char buffer[1024];
  sockaddr_in from_addr;
  socklen_t from_len = sizeof(from_addr);

  ssize_t received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                              (sockaddr*)&from_addr, &from_len);

  if (received < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      spdlog::debug("No data available (shouldn't happen after select)");
      return {SendResult::TIMEOUT, "No data available",
              std::chrono::milliseconds(0)};
    }
    spdlog::error("recvfrom failed: {}", strerror(errno));
    return {SendResult::NETWORK_ERROR, strerror(errno),
            std::chrono::milliseconds(0)};
  }

  if (received == 0) {
    spdlog::warn("Received 0 bytes from server");
    return {SendResult::INVALID_RESPONSE, "Empty response",
            std::chrono::milliseconds(0)};
  }

  // Verify response is from expected server
  if (from_addr.sin_addr.s_addr != server_addr.sin_addr.s_addr ||
      from_addr.sin_port != server_addr.sin_port) {
    char from_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &from_addr.sin_addr, from_ip, INET_ADDRSTRLEN);
    spdlog::warn("Received response from unexpected server: {}:{}", from_ip,
                 ntohs(from_addr.sin_port));
  }

  // Null-terminate response
  buffer[received] = '\0';
  std::string response_text(buffer, received);

  spdlog::trace("Received {} bytes: '{}'", received, response_text);

  // Validate response format (should be "created" or "rejected")
  if (response_text == "created" || response_text == "rejected") {
    return {SendResult::SUCCESS, response_text, std::chrono::milliseconds(0)};
  } else {
    spdlog::warn("Unexpected response format: '{}'", response_text);
    return {SendResult::INVALID_RESPONSE, response_text,
            std::chrono::milliseconds(0)};
  }
};