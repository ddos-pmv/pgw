#pragma once

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
      })
    }
  }

 private:
  void handleEvent(Event&& event) {}
  moodycamel::ConcurrentQueue<Event>& queue_;
  std::atomic_bool stop_;
  std::vector<std::thread> thread_pool_;
};

}  // namespace protei