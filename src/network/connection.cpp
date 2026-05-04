#include "network/connection.h"
#include <algorithm>
#include <string_view>
#include "utils/logger.h"

Connection::Connection(boost::asio::ip::tcp::socket socket, HashAlgorithm algo)
    : socket_(std::move(socket)),
      strand_(boost::asio::make_strand(socket_.get_executor())),
      timer_(socket_.get_executor()),
      hash_engine_(algo) {}

void Connection::start() {
  boost::system::error_code ec;

  // Optimization: Disable Nagle's algorithm for faster small-packet responses
  socket_.set_option(boost::asio::ip::tcp::no_delay(true), ec);

  // Reliability: Enable TCP Keep-Alive to detect half-open connections at the OS level
  socket_.set_option(boost::asio::socket_base::keep_alive(true), ec);

  try {
    LOG_INFO("Client connected: {}", socket_.remote_endpoint().address().to_string());
  } catch (...) {
    LOG_INFO("Client connected (endpoint info unavailable)");
  }

  // Initialize the inactivity watchdog and start reading
  reset_timeout();
  do_read();
}

void Connection::reset_timeout() {
  timer_.expires_after(IDLE_TIMEOUT_DURATION);

  auto self(shared_from_this());
  timer_.async_wait(boost::asio::bind_executor(strand_, [this, self](const boost::system::error_code& ec) {
    if (!ec) {
      LOG_WARN("Connection timed out. Closing socket.");
      stop();  // Use a dedicated stop method for clean shutdown
    }
  }));
}

void Connection::cancel_timeout() { timer_.cancel(); }

void Connection::stop() {
  boost::system::error_code ignored_ec;
  socket_.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ignored_ec);
  socket_.close(ignored_ec);
  timer_.cancel();
}

void Connection::do_read() {
  auto self(shared_from_this());
  socket_.async_read_some(
      boost::asio::buffer(buffer_),
      boost::asio::bind_executor(strand_, [this, self](boost::system::error_code ec, std::size_t length) {
        if (!ec) {
          cancel_timeout();
          reset_timeout();
          process_data(length);
        } else {
          // If ec is eof or any other error, we must stop the connection
          cancel_timeout();
          if (ec != boost::asio::error::operation_aborted) {
            LOG_INFO("Session finished: {}", ec.message());
            stop();  // This clears CLOSE_WAIT and releases the client
          }
        }
      }));
}

void Connection::process_data(std::size_t length) {
  std::string_view data(buffer_.data(), length);
  size_t pos = 0;
  size_t prev = 0;

  // Stream processing: Scan for newline characters to identify complete messages
  while ((pos = data.find('\n', prev)) != std::string_view::npos) {
    // Feed the chunk into the hash engine up to the newline
    hash_engine_.update(data.data() + prev, pos - prev);

    // Finalize hash for the current line and send it
    std::string hex_hash = hash_engine_.finalize();
    hex_hash += '\n';
    do_write(std::move(hex_hash));

    prev = pos + 1;
  }

  // Accumulate the remaining part (incomplete line) for the next read cycle
  if (prev < length) {
    hash_engine_.update(data.data() + prev, length - prev);
  }

  // Continue reading more data
  do_read();
}

void Connection::do_write(std::string response) {
  write_queue_.push_back(std::move(response));

  if (!is_writing_) {
    check_write_queue();
  }
}

void Connection::check_write_queue() {
  if (write_queue_.empty()) {
    is_writing_ = false;
    return;
  }

  is_writing_ = true;
  auto self(shared_from_this());

  boost::asio::async_write(socket_, boost::asio::buffer(write_queue_.front()),
                           boost::asio::bind_executor(strand_, [this, self](boost::system::error_code ec, std::size_t) {
                             if (!ec) {
                               write_queue_.pop_front();
                               check_write_queue();
                             } else if (ec != boost::asio::error::operation_aborted) {
                               LOG_ERROR("Write failed: {}", ec.message());
                               stop();
                             }
                           }));
}