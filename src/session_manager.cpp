#include "session_manager.h"

#include <pgw_utils/imsi.h>
#include <pgw_utils/logger_config.h>

namespace protei {

SessionManager::SessionManager(std::chrono::seconds timeout,
                               size_t shutdown_rate,
                               const std::unordered_set<std::string>& blacklist,
                               SessionEventCallback callback)
    : blacklist_(blacklist),
      session_timeout_(timeout),
      event_callback_(std::move(callback)),
      running_(true),
      graceful_shutdown_active_(false),
      shutdown_rate_(shutdown_rate) {
  cleanup_thread_ = std::thread(&SessionManager::cleanup_thread, this);
  spdlog::info("SessionManager started with: timout {}s, balcklist size {}",
               timeout.count(), blacklist_.size());
}

SessionManager::~SessionManager() {
  running_ = false;

  {
    std::lock_guard<std::mutex> lock(shutdown_mutex_);
    graceful_shutdown_active_ = false;
    shutdown_cv_.notify_all();
  }

  if (cleanup_thread_.joinable()) {
    cleanup_thread_.join();
  }

  if (graceful_shutdown_thread_.joinable()) {
    graceful_shutdown_thread_.join();
  }

  UDP_LOG_INFO("SessionManager destroyed, cleaned up {} sessions",
               sessions_.size());
}

SessionManager::CreateResult SessionManager::create_session(
    const std::string& imsi) {
  try {
    IMSI imsi_obj(imsi);
  } catch (const std::exception& e) {
    spdlog::warn("Invalid IMSI format: {}", imsi);
    return CreateResult::BLACKLISTED;  // Treat invalid IMSI as blacklisted
  }

  {
    std::shared_lock<std::shared_mutex> blacklist_lock(blacklist_mutex_);
    if (blacklist_.count(imsi)) {
      spdlog::warn("IMSI {} is blacklisted", imsi);
      emit_event(imsi, SessionAction::REJECTED);
      return CreateResult::BLACKLISTED;
    }
  }

  {
    std::unique_lock<std::shared_mutex> sessions_lock(sessions_mutex_);
    auto it = sessions_.find(imsi);

    if (it != sessions_.end()) {
      // Update existing session activity
      it->second->last_activity = std::chrono::steady_clock::now();
      spdlog::debug("Updated existing session for IMSI: {}", imsi);
      return CreateResult::ALREADY_EXISTS;
    }

    // Create new session
    sessions_[imsi] = std::make_unique<Session>(imsi);
    spdlog::info("Created new session for IMSI: {}", imsi);
    emit_event(imsi, SessionAction::CREATED);
    return CreateResult::CREATED;
  }
}

bool SessionManager::session_exists(const std::string& imsi) const {
  std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
  return sessions_.count(imsi) > 0;
}

bool SessionManager::remove_session(const std::string& imsi) {
  std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
  auto it = sessions_.find(imsi);
  if (it != sessions_.end()) {
    sessions_.erase(it);
    spdlog::debug("Manually removed session for IMSI: {}", imsi);
    return true;
  }
  return false;
}

void SessionManager::add_to_blacklist(const std::string& imsi) {
  std::unique_lock<std::shared_mutex> lock(blacklist_mutex_);
  blacklist_.insert(imsi);
  spdlog::info("Added IMSI {} to blacklist", imsi);
}

void SessionManager::remove_from_blacklist(const std::string& imsi) {
  std::unique_lock<std::shared_mutex> lock(blacklist_mutex_);
  blacklist_.erase(imsi);
  spdlog::info("Removed IMSI {} from blacklist", imsi);
}

bool SessionManager::is_blacklisted(const std::string& imsi) const {
  std::shared_lock<std::shared_mutex> lock(blacklist_mutex_);
  return blacklist_.count(imsi) > 0;
}

void SessionManager::start_graceful_shutdown() {
  graceful_shutdown_active_ = true;

  graceful_shutdown_thread_ =
      std::thread(&SessionManager::graceful_shutdown_thread, this);
}

void SessionManager::update_session_activity(const std::string& imsi) {
  std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
  auto it = sessions_.find(imsi);
  if (it != sessions_.end()) {
    it->second->last_activity = std::chrono::steady_clock::now();
  }
}

void SessionManager::cleanup_thread() {
  while (running_) {
    std::vector<std::string> expired_sessions;
    auto now = std::chrono::steady_clock::now();

    {
      std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
      for (const auto& [imsi, session] : sessions_) {
        if (now - session->last_activity >= session_timeout_) {
          expired_sessions.push_back(imsi);
        }
      }
    }

    if (!expired_sessions.empty()) {
      std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
      for (const auto& imsi : expired_sessions) {
        auto it = sessions_.find(imsi);
        if (it != sessions_.end() &&
            now - it->second->last_activity >= session_timeout_) {
          sessions_.erase(it);

          spdlog::debug("Session timeout for IMSI: {}", imsi);

          emit_event(imsi, SessionAction::TIMEOUT);
        }
      }
      if (!expired_sessions.empty()) {
        spdlog::info("Cleaned up {} expired sessions", expired_sessions.size());
      }
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }
}

void SessionManager::graceful_shutdown_thread() {
  const auto sleep_duration = std::chrono::milliseconds(1000 / shutdown_rate_);

  while (graceful_shutdown_active_ && running_) {
    std::string imsi_to_remove;

    {
      std::shared_lock<std::shared_mutex> lock(sessions_mutex_);
      if (!sessions_.empty()) {
        imsi_to_remove = sessions_.begin()->first;
      } else {
        // No more sessions, shutdown complete
        graceful_shutdown_active_ = false;
        UDP_LOG_INFO("Graceful shutdown completed - all sessions removed");
        break;
      }
    }

    // Remove session
    if (!imsi_to_remove.empty()) {
      {
        std::unique_lock<std::shared_mutex> lock(sessions_mutex_);
        auto it = sessions_.find(imsi_to_remove);
        if (it != sessions_.end()) {
          sessions_.erase(it);
          emit_event(imsi_to_remove, SessionAction::GRACEFUL_SHUTDOWN);
        }
      }

      // Rate limiting
      std::unique_lock<std::mutex> shutdown_lock(shutdown_mutex_);
      shutdown_cv_.wait_for(shutdown_lock, sleep_duration,
                            [this] { return !graceful_shutdown_active_; });
    }
  }
}

void SessionManager::emit_event(const std::string& imsi, SessionAction action) {
  if (event_callback_) {
    SessionEvent event{imsi, action, std::chrono::system_clock::now()};
    event_callback_(event);
  }
}
}  // namespace protei