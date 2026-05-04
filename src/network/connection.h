#pragma once

#include <array>
#include <boost/asio.hpp>
#include <deque>
#include <memory>
#include <string>
#include "hash/hash_engine.h"

class Connection : public std::enable_shared_from_this<Connection> {
 public:
  Connection(boost::asio::ip::tcp::socket socket, HashAlgorithm algo);
  void start();

 private:
  void do_read();
  void stop();
  void process_data(std::size_t length);
  void do_write(std::string response);
  void check_write_queue();

  void reset_timeout();
  void cancel_timeout();

  boost::asio::ip::tcp::socket socket_;
  boost::asio::strand<boost::asio::any_io_executor> strand_;
  boost::asio::steady_timer timer_;

  HashEngine hash_engine_;
  std::array<char, 65536> buffer_;
  std::deque<std::string> write_queue_;
  bool is_writing_ = false;

  static constexpr std::chrono::seconds IDLE_TIMEOUT_DURATION{30};
};