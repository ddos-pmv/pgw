#include "pgw_utils/logger_config.h"

#include <iostream>
#include <memory>
#include <vector>

namespace protei {

void LoggerConfig::configure_logger(const std::string& log_file,
                                    const std::string& log_level,
                                    const std::string& logger_name) {
  try {
    SharedConsoleSink console_sink = get_shared_console_sink();
    SharedFileSink file_sink =
        std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, 1024 * 1024 * 10, 5);  // 10MB, 5 files

    console_sink->set_level(string_to_level(log_level));
    console_sink->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] [tid:%t] %v");

    file_sink->set_level(string_to_level(log_level));
    file_sink->set_pattern(
        "[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [tid:%t] [%s:%#] %v");
    register_file_sink(log_file, file_sink);

    std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
    SharedLogger logger = std::make_shared<spdlog::logger>(
        logger_name, sinks.begin(), sinks.end());

    logger->set_level(string_to_level(log_level));
    logger->flush_on(spdlog::level::warn);

    spdlog::set_default_logger(logger);
    spdlog::set_level(string_to_level(log_level));

    spdlog::info("Default logger configured: file={}, level={}", log_file,
                 log_level);

  } catch (const spdlog::spdlog_ex& ex) {
    std::cerr << "Log initialization failed: " << ex.what() << std::endl;
    throw;
  }
}

void LoggerConfig::configure_component_logger(const std::string& component_name,
                                              const std::string& log_file,
                                              const std::string& log_level) {
  try {
    SharedConsoleSink console_sink = get_shared_console_sink();
    SharedFileSink file_sink;

    SharedFileSink existing_sink = find_existing_file_sink(log_file);
    if (existing_sink) {
      file_sink = existing_sink;
      spdlog::debug("Reusing existing file sink for: {}", log_file);
    } else {
      file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
          log_file, 1024 * 1024 * 5, 3);  // 5MB, 3 files

      file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [tid:%t] %v");

      register_file_sink(log_file, file_sink);
      spdlog::debug("Created new file sink for: {}", log_file);
    }

    // Set sink level to minimum of existing and new logger levels
    auto current_level =
        std::min(file_sink->level(), string_to_level(log_level));
    file_sink->set_level(current_level);
    auto current_console_leve =
        std::min(console_sink->level(), string_to_level(log_level));
    console_sink->set_level(current_console_leve);

    std::vector<spdlog::sink_ptr> sinks{file_sink, console_sink};
    SharedLogger logger = std::make_shared<spdlog::logger>(
        component_name, sinks.begin(), sinks.end());
    logger->set_level(string_to_level(log_level));
    logger->flush_on(spdlog::level::info);

    spdlog::register_logger(logger);

    spdlog::info("Component logger '{}' configured: file={}, level={}",
                 component_name, log_file, log_level);

  } catch (const spdlog::spdlog_ex& ex) {
    spdlog::error("Failed to configure component logger '{}': {}",
                  component_name, ex.what());
    throw;
  }
}

LoggerConfig::FileSinkRegistry& LoggerConfig::get_file_sinks_registry() {
  static FileSinkRegistry registry;
  return registry;
}

void LoggerConfig::register_file_sink(const std::string& log_file,
                                      SharedFileSink sink) {
  get_file_sinks_registry()[log_file] = WeakFileSink(sink);
}

LoggerConfig::SharedFileSink LoggerConfig::find_existing_file_sink(
    const std::string& log_file) {
  FileSinkRegistry& registry = get_file_sinks_registry();
  auto it = registry.find(log_file);

  if (it != registry.end()) {
    SharedFileSink sink = it->second.lock();
    if (sink) {
      return sink;
    } else {
      // Weak pointer expired, remove from registry
      registry.erase(it);
    }
  }
  return nullptr;
}

LoggerConfig::SharedConsoleSink LoggerConfig::get_shared_console_sink() {
  static SharedConsoleSink console_sink = nullptr;

  if (!console_sink) {
    console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(spdlog::level::trace);
    console_sink->set_pattern("[%H:%M:%S.%e] [%n] [%^%l%$] [tid:%t] %v");
  }

  return console_sink;
}

spdlog::level::level_enum LoggerConfig::string_to_level(
    const std::string& level) {
  if (level == "TRACE") return spdlog::level::trace;
  if (level == "DEBUG") return spdlog::level::debug;
  if (level == "INFO") return spdlog::level::info;
  if (level == "WARN") return spdlog::level::warn;
  if (level == "ERROR") return spdlog::level::err;
  if (level == "CRITICAL") return spdlog::level::critical;
  if (level == "OFF") return spdlog::level::off;

  spdlog::warn("Unknown log level '{}', defaulting to INFO", level);
  return spdlog::level::info;
}

}  // namespace protei