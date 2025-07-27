#pragma once

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string>
#include <unordered_map>

namespace protei {

class LoggerConfig {
 public:
  // Type aliases для улучшения читаемости
  using SharedFileSink = std::shared_ptr<spdlog::sinks::rotating_file_sink_mt>;
  using WeakFileSink = std::weak_ptr<spdlog::sinks::rotating_file_sink_mt>;
  using FileSinkRegistry = std::unordered_map<std::string, WeakFileSink>;
  using SharedConsoleSink =
      std::shared_ptr<spdlog::sinks::stdout_color_sink_mt>;
  using SharedLogger = std::shared_ptr<spdlog::logger>;

  static void configure_logger(const std::string& log_file,
                               const std::string& log_level,
                               const std::string& logger_name = "pgw");

  static void configure_component_logger(const std::string& component_name,
                                         const std::string& log_file,
                                         const std::string& log_level);

  // get logger by name
  static SharedLogger get_logger(const std::string& name) {
    return spdlog::get(name);
  }

  // Check if logger exists
  static bool logger_exists(const std::string& name) {
    return spdlog::get(name) != nullptr;
  }

 private:
  static FileSinkRegistry& get_file_sinks_registry();

  static void register_file_sink(const std::string& log_file,
                                 SharedFileSink sink);

  static SharedFileSink find_existing_file_sink(const std::string& log_file);

  static SharedConsoleSink get_shared_console_sink();

  static spdlog::level::level_enum string_to_level(const std::string& level);
};

// Convenience macros for component-specific logging
#define UDP_LOG_TRACE(...)                                   \
  if (auto logger = protei::LoggerConfig::get_logger("udp")) \
  logger->trace(__VA_ARGS__)
#define UDP_LOG_DEBUG(...)                                   \
  if (auto logger = protei::LoggerConfig::get_logger("udp")) \
  logger->debug(__VA_ARGS__)
#define UDP_LOG_INFO(...)                                    \
  if (auto logger = protei::LoggerConfig::get_logger("udp")) \
  logger->info(__VA_ARGS__)
#define UDP_LOG_WARN(...)                                    \
  if (auto logger = protei::LoggerConfig::get_logger("udp")) \
  logger->warn(__VA_ARGS__)
#define UDP_LOG_ERROR(...)                                   \
  if (auto logger = protei::LoggerConfig::get_logger("udp")) \
  logger->error(__VA_ARGS__)

#define HTTP_LOG_TRACE(...)                                   \
  if (auto logger = protei::LoggerConfig::get_logger("http")) \
  logger->trace(__VA_ARGS__)
#define HTTP_LOG_DEBUG(...)                                   \
  if (auto logger = protei::LoggerConfig::get_logger("http")) \
  logger->debug(__VA_ARGS__)
#define HTTP_LOG_INFO(...)                                    \
  if (auto logger = protei::LoggerConfig::get_logger("http")) \
  logger->info(__VA_ARGS__)
#define HTTP_LOG_WARN(...)                                    \
  if (auto logger = protei::LoggerConfig::get_logger("http")) \
  logger->warn(__VA_ARGS__)
#define HTTP_LOG_ERROR(...)                                   \
  if (auto logger = protei::LoggerConfig::get_logger("http")) \
  logger->error(__VA_ARGS__)
}  // namespace protei