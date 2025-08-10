#include "event_dispatcher.h"

#include <pgw_utils/imsi.h>
#include <pgw_utils/logger_config.h>
namespace protei {

EventDispatcher::EventDispatcher(
    size_t thread_count, moodycamel::ConcurrentQueue<Event>& queue,
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
          continue;
        }

        // std::this_thread::yield();

        std::unique_lock<std::mutex> lock(notify_mutex_);
        notify_cv_.wait(
            lock, [this] { return event_available_.load() || stop_.load(); });

        event_available_ = false;
      }
    });
  }
  spdlog::info("EventDispatcher startedwith {} threads", thread_count);
}

EventDispatcher::~EventDispatcher() { stop(); }

void EventDispatcher::notify_event_available() {
  event_available_ = true;
  notify_cv_.notify_one();
}

void EventDispatcher::handleEvent(Event&& event) {
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

void EventDispatcher::handleUdpEvent(UdpEvent&& event) {
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
        UDP_LOG_INFO("Session rejected (blacklisted) for IMSI: {}", imsi.str());
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

void EventDispatcher::stop() {
  if (stop_.exchange(false)) {
    return;
  }

  spdlog::info("Stopping EventDispatcher ...");

  notify_cv_.notify_all();

  for (auto& thread : thread_pool_) {
    if (thread.joinable()) {
      thread.join();
    }
  }

  spdlog::info("EventDispatcher stoped");
}

}  // namespace protei