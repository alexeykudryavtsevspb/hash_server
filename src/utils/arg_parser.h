#pragma once

#include "hash/hash_algorithm.h"

#include <optional>
#include <string>

struct ServerConfig {
  std::string host = "0.0.0.0";
  unsigned short port = 12345;
  bool verbose = false;
  HashAlgorithm algo = HashAlgorithm::SHA256;  // Default
};

struct ParseResult {
  enum class Status { Success, Help, Error };

  Status status;
  std::optional<ServerConfig> config;
  int exit_code = 0;
};

namespace utils {
ParseResult parse_arguments(int argc, char* argv[]);
}  // namespace utils
