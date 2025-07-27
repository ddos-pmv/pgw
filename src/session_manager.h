#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_set>

namespace protei {

enum class SessionAction { CREATED, REJECTED, TIMEOUT, GRACEFUL_SHUTDOWN };
struct SessionEvent {
  std::string imsi;
  SessionAction action;
  std::chrono::system_clock::time_point timestamp;
};

struct Session {
  std::string imsi;
  std::chrono::steady_clock::time_point created_at;
  std::chrono::steady_clock::time_point last_activity;

  explicit Session(const std::string& imsi_str)
      : imsi(imsi_str),
        created_at(std::chrono::steady_clock::now()),
        last_activity(std::chrono::steady_clock::now()) {}
};

class SessionManager {
 public:
  using SessionEventCallback = std::function<void(const SessionEvent&)>;

  explicit SessionManager(std::chrono::seconds timeout, size_t shutdown_rate,
                          const std::unordered_set<std::string>& blacklist = {},
                          SessionEventCallback callback = nullptr);

  ~SessionManager();

  // Main session operations
  enum class CreateResult { CREATED, ALREADY_EXISTS, BLACKLISTED };
  CreateResult create_session(const std::string& imsi);

  bool session_exists(const std::string& imsi) const;
  bool remove_session(const std::string& imsi);

  // Blacklist management
  void add_to_blacklist(const std::string& imsi);
  void remove_from_blacklist(const std::string& imsi);
  bool is_blacklisted(const std::string& imsi) const;

  // Statistics
  //   size_t active_sessions_count() const;
  //   std::vector<std::string> get_active_sessions() const;

  // Graceful shutdown with configurable rate
  void start_graceful_shutdown();
  bool is_shutting_down() const { return graceful_shutdown_active_; }

  // Update activity for existing session
  void update_session_activity(const std::string& imsi);

 private:
  void cleanup_thread();
  void graceful_shutdown_thread();
  void emit_event(const std::string& imsi, SessionAction action);

  mutable std::shared_mutex sessions_mutex_;
  mutable std::shared_mutex blacklist_mutex_;

  std::unordered_map<std::string, std::unique_ptr<Session>> sessions_;
  std::unordered_set<std::string> blacklist_;

  std::chrono::seconds session_timeout_;
  SessionEventCallback event_callback_;

  // Background threads
  std::atomic<bool> running_;
  std::atomic<bool> graceful_shutdown_active_;
  std::thread cleanup_thread_;
  std::thread graceful_shutdown_thread_;

  // Graceful shutdown parameters
  std::atomic<size_t> shutdown_rate_;  // sessions per second
  mutable std::condition_variable shutdown_cv_;
  mutable std::mutex shutdown_mutex_;
};

}  // namespace protei
