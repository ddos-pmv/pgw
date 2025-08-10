#pragma once
#include <spdlog/spdlog.h>

#include "session_manager.h"

namespace protei {

// enum class SessionAction;
// struct SessionEvent;

class CDRWriter {
 public:
  explicit CDRWriter(const std::string& filename);

  ~CDRWriter();
  CDRWriter(const CDRWriter&) = delete;
  CDRWriter& operator=(const CDRWriter&) = delete;
  CDRWriter(CDRWriter&&) = default;
  CDRWriter& operator=(CDRWriter&&) = default;

  // Write CDR record (thread-safe via spdlog)
  void write_record(const std::string& imsi, SessionAction action);

  void write_record(const SessionEvent& event);

  size_t total_records_written() const;

  void flush();

 private:
  std::string format_record(
      const std::string& imsi, SessionAction action,
      const std::chrono::system_clock::time_point& timestamp) const;

  std::string action_to_string(SessionAction action) const;

  std::string format_timestamp(
      const std::chrono::system_clock::time_point& tp) const;

  std::shared_ptr<spdlog::logger> cdr_logger_;
  std::atomic<size_t> total_records_{0};
};

inline auto create_cdr_callback(CDRWriter& cdr_writer) {
  return [&cdr_writer](const SessionEvent& event) {
    cdr_writer.write_record(event);
  };
}

}  // namespace protei