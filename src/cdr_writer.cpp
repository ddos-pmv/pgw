#include "cdr_writer.h"

#include <spdlog/sinks/rotating_file_sink.h>

#include "session_manager.h"

namespace protei {

// enum class SessionAction;
// struct SessionEvent;

CDRWriter::CDRWriter(const std::string& filename) {
  auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
      filename, 1024 * 1024 * 50, 10);

  cdr_logger_ = std::make_shared<spdlog::logger>("cdr", file_sink);
  cdr_logger_->set_level(spdlog::level::info);
  cdr_logger_->set_pattern("%v");
  cdr_logger_->flush_on(spdlog::level::info);

  spdlog::register_logger(cdr_logger_);

  // Write CDR header
  cdr_logger_->info("# CDR Log - Format: timestamp,IMSI,action");

  total_records_ = 0;
}

CDRWriter::~CDRWriter() {
  if (cdr_logger_) {
    cdr_logger_->flush();
  }
}

// Write CDR record (thread-safe via spdlog)
void CDRWriter::write_record(const std::string& imsi, SessionAction action) {
  if (!cdr_logger_) return;

  auto now = std::chrono::system_clock::now();
  std::string record = format_record(imsi, action, now);
  cdr_logger_->info(record);
  total_records_++;
}

void CDRWriter::write_record(const SessionEvent& event) {
  if (!cdr_logger_) return;

  std::string record = format_record(event.imsi, event.action, event.timestamp);
  cdr_logger_->info(record);
  total_records_++;
}

size_t CDRWriter::total_records_written() const { return total_records_; }

void CDRWriter::flush() {
  if (cdr_logger_) {
    cdr_logger_->flush();
  }
}

std::string CDRWriter::format_record(
    const std::string& imsi, SessionAction action,
    const std::chrono::system_clock::time_point& timestamp) const {
  return format_timestamp(timestamp) + "," + imsi + "," +
         action_to_string(action);
}

std::string CDRWriter::action_to_string(SessionAction action) const {
  switch (action) {
    case SessionAction::CREATED:
      return "CREATED";
    case SessionAction::REJECTED:
      return "REJECTED";
    case SessionAction::TIMEOUT:
      return "TIMEOUT";
    case SessionAction::GRACEFUL_SHUTDOWN:
      return "GRACEFUL_SHUTDOWN";
    default:
      return "UNKNOWN";
  }
}

std::string CDRWriter::format_timestamp(
    const std::chrono::system_clock::time_point& tp) const {
  auto time_t = std::chrono::system_clock::to_time_t(tp);
  auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                tp.time_since_epoch()) %
            1000;

  char buffer[32];
  std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S",
                std::gmtime(&time_t));

  return std::string(buffer) + "." + (ms.count() < 100 ? "0" : "") +
         (ms.count() < 10 ? "0" : "") + std::to_string(ms.count());
}

std::shared_ptr<spdlog::logger> cdr_logger_;
std::atomic<size_t> total_records_{0};

}  // namespace protei