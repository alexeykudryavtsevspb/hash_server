#pragma once

#include <spdlog/fmt/ostr.h>  // For logging custom types if needed
#include <spdlog/spdlog.h>

namespace Logger {
void init(bool verbose);
}  // namespace Logger

// Macros for zero-overhead logging when level is disabled
#define LOG_INFO(...) \
  if (spdlog::should_log(spdlog::level::info)) spdlog::info(__VA_ARGS__)
#define LOG_WARN(...) \
  if (spdlog::should_log(spdlog::level::warn)) spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...) \
  if (spdlog::should_log(spdlog::level::err)) spdlog::error(__VA_ARGS__)
#define LOG_CRIT(...) \
  if (spdlog::should_log(spdlog::level::critical)) spdlog::critical(__VA_ARGS__)
