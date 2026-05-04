#include <iostream>

#include "network/tcp_server.h"
#include "utils/arg_parser.h"
#include "utils/logger.h"

int main(int argc, char* argv[]) {
  // 1. config
  auto result = utils::parse_arguments(argc, argv);
  if (result.status != ParseResult::Status::Success) {
    return result.exit_code;
  }
  const auto& config = *result.config;
  try {
    // 2. logger
    Logger::init(config.verbose);
    // 3. server
    TcpServer server(config.host, config.port, config.algo);
    server.run();
    LOG_INFO("Server started on {}:{}", config.host, config.port);
    server.wait();
  } catch (const std::exception& e) {
    std::cerr << "Fatal Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}