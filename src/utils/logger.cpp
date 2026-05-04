#include "utils/logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace Logger {

void init(bool verbose) {
  // Create a colorized stdout sink
  auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

  // Set pattern: [Timestamp] [Level] [Thread ID] Message
  console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [thread %t] %v");

  auto logger = std::make_shared<spdlog::logger>("server_logger", console_sink);
  spdlog::set_default_logger(logger);

  if (verbose) {
    spdlog::set_level(spdlog::level::info);
  } else {
    spdlog::set_level(spdlog::level::err);
  }

  spdlog::flush_on(spdlog::level::info);
}

}  // namespace Logger