#pragma once

#include <pgw_utils/logger_config.h>

#include <iostream>
#include <variant>

#include "concurrentqueue.h"
#include "event.h"

namespace protei {

class EventDispatcher {
 public:
  EventDispatcher(size_t thread_count,
                  moodycamel::ConcurrentQueue<Event>& queue)
      : queue_(queue), stop_(false) {
    for (size_t i = 0; i < thread_count; i++) {
      thread_pool_.emplace_back([this]() {
        Event event;
        while (!stop_) {
          if (queue_.try_dequeue(event)) {
            handleEvent(std::move(event));
          } else {
            std::this_thread::yield();
          }
        }
      });
    }
    spdlog::info("EventDispatcher startedwith {} threads", thread_count);
  }

  ~EventDispatcher() { stop(); }

 private:
  void handleEvent(Event&& event) {
    std::visit(
        [this](auto&& arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, UdpEvent>) {
            handleUdpEvent(std::move(arg));
          }
          if constexpr (std::is_same_v<T, HttpEvent>) {
          }
        },
        event);
  }

  void handleUdpEvent(UdpEvent&& event) {
    UDP_LOG_TRACE("Porcessing UDP event with {} bytes", event.data.size());

    try {
      IMSI imsi = IMSI::from_bcd(event.data.data(), event.data.size());

    } catch (...) {
    }
  }

  void stop() {
    if (stop_.exchange(false)) {
      return;
    }

    spdlog::info("Stopping EventDispatcher ...");

    for (auto& thread : thread_pool_) {
      if (thread.joinable()) {
        thread.join();
      }
    }

    spdlog::info("EventDispatcher stoped");
  }
  moodycamel::ConcurrentQueue<Event>& queue_;
  std::atomic_bool stop_;
  std::vector<std::thread> thread_pool_;
};

}  // namespace protei