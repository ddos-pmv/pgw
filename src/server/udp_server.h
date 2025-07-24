#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <queue>
#include <mutex>
#include <unordered_set>
#include <condition_variable>
#include <unordered_map>

#include <cstdint>


#include <pgw_utils/unique_fd.h>

namespace protei
{
  class Server
  {
  public:
    Server(uint16_t port, int thread_count = 12);

    void start();

  private:
    void worker_thread();

    void accept_new_connection();

    // void safe_write(int fd, const std::string &data);

    uint16_t port_;
    UniqueFd epoll_fd_;
    UniqueFd server_fd_;

    std::mutex map_mtx_;
    std::unordered_set<int> busy_clients_;
    std::mutex set_mtx_;

    int thread_count_;
    std::vector<std::thread>
        thread_pool_;
    alignas(64) std::condition_variable cv_;
    alignas(64) std::mutex queue_mtx_;
    alignas(64) std::atomic_bool stop_;
  };
} // namespace protei
