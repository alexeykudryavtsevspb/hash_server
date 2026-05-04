#include "utils/arg_parser.h"
#include <algorithm>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace utils {

class ArgumentError : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

// Helper to map string names to enum values
static HashAlgorithm string_to_algo(std::string name) {
  static const std::unordered_map<std::string, HashAlgorithm> algo_map = {
      {"sha256", HashAlgorithm::SHA256}, {"md5", HashAlgorithm::MD5}, {"sha512", HashAlgorithm::SHA512}};

  std::transform(name.begin(), name.end(), name.begin(), ::tolower);

  auto it = algo_map.find(name);
  if (it == algo_map.end()) {
    throw ArgumentError("Unsupported algorithm: " + name);
  }
  return it->second;
}

void print_usage(const char* prog_name) {
  std::cout << "Usage: " << prog_name << " [host:port] [options]\n"
            << "Options:\n"
            << "  -v, --verbose          Enable detailed logging\n"
            << "  -a, --algo <name>      Hash algorithm: sha256 (default), md5, sha512\n"
            << "  -h, --help             Show this help message\n"
            << "Example:\n"
            << "  " << prog_name << " 0.0.0.0:8080 --algo md5 --verbose\n";
}

ParseResult parse_arguments(int argc, char* argv[]) {
  ParseResult result;
  ServerConfig config;
  std::string prog_name = (argc > 0) ? argv[0] : "hash_server";

  try {
    std::vector<std::string> args(argv + 1, argv + argc);

    for (size_t i = 0; i < args.size(); ++i) {
      const std::string& arg = args[i];

      if (arg == "-v" || arg == "--verbose") {
        config.verbose = true;
      } else if (arg == "-h" || arg == "--help") {
        print_usage(prog_name.c_str());
        result.status = ParseResult::Status::Help;
        result.exit_code = 0;
        return result;
      } else if (arg == "-a" || arg == "--algo") {
        if (i + 1 >= args.size()) {
          throw ArgumentError("Missing value for --algo");
        }
        std::string algo_name = args[++i];
        config.algo = string_to_algo(algo_name);
      } else if (arg.find('.') != std::string::npos || arg.find(':') != std::string::npos || arg == "localhost") {
        auto pos = arg.find(':');
        try {
          if (pos != std::string::npos) {
            config.host = arg.substr(0, pos);
            std::string port_str = arg.substr(pos + 1);
            if (!port_str.empty()) {
              int p = std::stoi(port_str);
              if (p < 1 || p > 65535) throw std::out_of_range("port");
              config.port = static_cast<unsigned short>(p);
            }
          } else {
            config.host = arg;
          }
        } catch (...) {
          throw ArgumentError("Invalid address or port range: " + arg);
        }
      } else {
        throw ArgumentError("Unknown argument: " + arg);
      }
    }

    result.status = ParseResult::Status::Success;
    result.config = config;
    return result;

  } catch (const ArgumentError& e) {
    std::cerr << "Configuration Error: " << e.what() << "\n";
    print_usage(prog_name.c_str());
    result.status = ParseResult::Status::Error;
    result.exit_code = 2;  // Common exit code for CLI usage errors
    return result;
  } catch (const std::exception& e) {
    std::cerr << "Unexpected parsing error: " << e.what() << "\n";
    result.status = ParseResult::Status::Error;
    result.exit_code = 1;
    return result;
  }
}

}  // namespace utils