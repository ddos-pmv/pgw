#pragma once

#include <string>
#include <variant>

namespace protei {
struct UdpEvent {
  std::string data;
};
struct HttpEvent {
  std::string body;
  std::string path;
};

using Event = std::variant<UdpEvent, HttpEvent>;
}  // namespace protei