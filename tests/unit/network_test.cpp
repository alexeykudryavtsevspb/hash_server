#include <gtest/gtest.h>
#include <boost/asio.hpp>
#include <chrono>
#include <thread>
#include "network/tcp_server.h"

using boost::asio::ip::tcp;

class NetworkTest : public ::testing::Test {
 protected:
  const std::string host = "0.0.0.0";
  const uint16_t port = 12349;

  // Helper method that accesses private members of TcpServer
  void stop_server(TcpServer& server) { server.io_context_.stop(); }
};

// Test 1: Basic connection and hash retrieval
TEST_F(NetworkTest, ServerCalculatesCorrectHash) {
  TcpServer server(host, port, HashAlgorithm::MD5);

  std::jthread server_thread([&] {
    server.run();
    server.wait();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  try {
    boost::asio::io_context client_ctx;
    tcp::socket socket(client_ctx);
    socket.connect(tcp::endpoint(boost::asio::ip::make_address(host), port));

    std::string msg = "hello\n";
    boost::asio::write(socket, boost::asio::buffer(msg));

    boost::asio::streambuf receive_buffer;
    boost::asio::read_until(socket, receive_buffer, '\n');

    std::string response = boost::asio::buffer_cast<const char*>(receive_buffer.data());
    response.erase(response.find_last_not_of(" \n\r\t") + 1);

    EXPECT_EQ(response, "5d41402abc4b2a76b9719d911017c592");
  } catch (...) {
    stop_server(server);
    throw;
  }

  stop_server(server);
}

// Test 2: Handling empty input
TEST_F(NetworkTest, HandlesEmptyInput) {
  TcpServer server(host, port + 1, HashAlgorithm::SHA256);

  std::jthread server_thread([&] {
    server.run();
    server.wait();
  });

  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  boost::asio::io_context client_ctx;
  tcp::socket socket(client_ctx);
  socket.connect(tcp::endpoint(boost::asio::ip::make_address(host), port + 1));

  boost::asio::write(socket, boost::asio::buffer("\n"));

  boost::asio::streambuf receive_buffer;
  boost::asio::read_until(socket, receive_buffer, '\n');

  stop_server(server);
  // Success if we reached here without hanging
}
