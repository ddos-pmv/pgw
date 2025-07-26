#pragma once

#include <iostream>
#include <variant>

#include "concurrentqueue.h"
#include "event.h"

namespace protei {

class EventDisptacher {
 public:
  EventDisptacher(size_t thread_count,
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
  }

 private:
  void handleEvent(Event&& event) {
    std::visit(
        [this](auto&& arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, UdpEvent>) {
            std::cout << "[tid:" << std::this_thread::get_id() << "]"
                      << "PROCCESS UDP EVENT" << std::endl;
          }
        },
        event);
  }
  moodycamel::ConcurrentQueue<Event>& queue_;
  std::atomic_bool stop_;
  std::vector<std::thread> thread_pool_;
};

}  // namespace protei