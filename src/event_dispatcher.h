#pragma once

#include <pgw_utils/logger_config.h>

#include <iostream>
#include <memory>
#include <variant>

#include "concurrentqueue.h"
#include "event.h"
#include "session_manager.h"

namespace protei {

class EventDispatcher {
 public:
  EventDispatcher(size_t thread_count,
                  moodycamel::ConcurrentQueue<Event>& queue,
                  std::shared_ptr<SessionManager> session_manager)
      : queue_(queue),
        stop_(false),
        session_manager_(std::move(session_manager)) {
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

      UDP_LOG_TRACE("Decoded IMSI: {}", imsi.str());
      auto result = session_manager_->create_session(imsi.str());

      std::string response;
      switch (result) {
        case SessionManager::CreateResult::CREATED:
          response = "created";
          UDP_LOG_INFO("Session created for IMSI: {}", imsi.str());
          break;

        case SessionManager::CreateResult::ALREADY_EXISTS:
          response =
              "created";  // Still respond with created for existing sessions
          UDP_LOG_DEBUG("Session already exists for IMSI: {}", imsi.str());
          break;

        case SessionManager::CreateResult::BLACKLISTED:
          response = "rejected";
          UDP_LOG_INFO("Session rejected (blacklisted) for IMSI: {}",
                       imsi.str());
          break;
      }

      // Send responce in this thread
      if (event.response_callback) {
        event.response_callback(response, event.client_addr);
      }
    } catch (const std::exception& e) {
      UDP_LOG_ERROR("Error processing UDP event: {}", e.what());

      // Send error response
      if (event.response_callback) {
        event.response_callback("rejected", event.client_addr);
      }
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
  std::shared_ptr<SessionManager> session_manager_;
};

}  // namespace protei