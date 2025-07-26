#pragma once

#include <pgw_utils/unique_fd.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "concurrentqueue.h"
#include "event.h"

namespace protei {
class UdpServer {
 public:
  UdpServer(uint16_t port, moodycamel::ConcurrentQueue<Event>& queue_);

  void start();

 private:
  void worker_thread();

  void accept_new_connection();

  // void safe_write(int fd, const std::string &data);

  uint16_t port_;
  UniqueFd epoll_fd_;
  UniqueFd server_fd_;
};
}  // namespace protei
