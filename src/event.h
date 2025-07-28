#pragma once

#include <netinet/in.h>

#include <functional>
#include <variant>
#include <vector>

namespace protei {
struct UdpEvent {
  std::vector<uint8_t> data;
  sockaddr_in client_addr;
  std::function<void(const std::string &, const sockaddr_in &)>
      response_callback;
};

struct HttpEvent {
  std::string path;
  std::string method;
  std::string body;
  std::function<void(int status, const std::string &response)>
      response_callback;
};

using Event = std::variant<UdpEvent, HttpEvent>;
}  // namespace protei