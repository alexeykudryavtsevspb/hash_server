#include "network/tcp_server.h"
#include "network/connection.h"
#include "utils/logger.h"

TcpServer::TcpServer(const std::string& address, unsigned short port, HashAlgorithm algo)
    : algo_(algo), io_context_(), signals_(io_context_), acceptor_(io_context_) {
  auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::make_address(address), port);

  acceptor_.open(endpoint.protocol());
  acceptor_.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
  acceptor_.bind(endpoint);
  acceptor_.listen();

  signals_.add(SIGINT);
  signals_.add(SIGTERM);

  // Graceful shutdown: stop accepting new connections, let existing ones finish
  signals_.async_wait([this](auto, int) {
    LOG_INFO("Shutdown signal received. Closing acceptor...");
    acceptor_.close();
  });

  LOG_INFO("Server listening on {}:{}", address, port);
}

void TcpServer::run() {
  do_accept();

  unsigned int thread_count = std::thread::hardware_concurrency();
  if (thread_count == 0) thread_count = 2;

  for (unsigned int i = 0; i < thread_count; ++i) {
    thread_pool_.emplace_back([this] {
      while (!io_context_.stopped()) {
        try {
          io_context_.run();
          break;
        } catch (const std::exception& e) {
          LOG_ERROR("Worker thread recovered from error: {}", e.what());
        }
      }
    });
  }

  LOG_INFO("Server started with pool of {} threads", thread_count);
}

void TcpServer::do_accept() {
  acceptor_.async_accept([this](boost::system::error_code ec, boost::asio::ip::tcp::socket socket) {
    if (!ec) {
      try {
        // Safely attempt to create a connection.
        // Exceptions in HashEngine or Connection won't kill the worker thread.
        std::make_shared<Connection>(std::move(socket), algo_)->start();
      } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize client connection: {}", e.what());
        // Socket is automatically closed as it goes out of scope here
      }
    } else if (ec == boost::asio::error::operation_aborted) {
      LOG_INFO("Acceptor operation aborted (server stopping).");
      return;
    } else {
      LOG_ERROR("Accept error: {}", ec.message());
    }

    // Continue accepting new connections
    do_accept();
  });
}

void TcpServer::wait() {
  for (auto& t : thread_pool_) {
    if (t.joinable()) t.join();
  }
  LOG_INFO("Server stopped. All worker threads joined.");
}
