#pragma once

#include <pgw_utils/unique_fd.h>

#include <atomic>
#include <cstdint>

#include "concurrentqueue.h"
#include "event.h"

namespace protei {
class UdpServer {
 public:
  static constexpr size_t MAX_EVENTS = 1024;
  static constexpr size_t BUFFER_SIZE = 1024;
  static constexpr size_t EPOLL_TIMEOUT_MS = 100;

  UdpServer(uint16_t port, moodycamel::ConcurrentQueue<Event>& queue_);

  /**
   *
   * @brief Detached starting epoll
   */
  void start();

 private:
  void stop();

  void epoll_loop();

  void process_incoming_packets(std::vector<uint8_t>& buffer);

  // void safe_write(int fd, const std::string &data);

  std::atomic_bool running_;
  std::atomic_bool stop_;

  std::thread io_thread_;

  uint16_t port_;
  UniqueFd server_fd_;
  UniqueFd epoll_fd_;

  moodycamel::ConcurrentQueue<Event>& queue_;
};
}  // namespace protei
