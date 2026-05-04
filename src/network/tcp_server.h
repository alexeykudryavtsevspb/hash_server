#pragma once

#include "hash/hash_algorithm.h"

#include <boost/asio.hpp>

#include <string>
#include <thread>
#include <vector>

class TcpServer {
 public:
  TcpServer(const std::string& address, unsigned short port, HashAlgorithm algo);

  void run();
  void wait();

 private:
  void do_accept();

  HashAlgorithm algo_;
  boost::asio::io_context io_context_;
  boost::asio::signal_set signals_;
  boost::asio::ip::tcp::acceptor acceptor_;
  std::vector<std::jthread> thread_pool_;

  // Allowing direct access to private members for testing purposes (e.g., stopping the server)
  friend class NetworkTest;
};
