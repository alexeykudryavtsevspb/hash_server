#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <string>
#include <thread>
#include <vector>

// --- Global Test Settings ---
const char* SERVER_IP = "127.0.0.1";
const int SERVER_PORT = 12345;
const int TEST_DURATION_SEC = 5;
const size_t SEND_CHUNK_SIZE = 4096;

const std::vector<size_t> PACKET_SIZES = {
  //clang-format off
  64,
  1024,
  2 * 1024,
  4 * 1024,
  16 * 1024,
  64 * 1024,
  128 * 1024,
  1024 * 1024,
  1024 * 1024 * 1024
  //clang-format on
};

std::atomic<bool> keep_running(true);

// Error types for reporting
enum class ErrorType { CONNECTION_LOST, TIMEOUT, INVALID_FORMAT, SEND_FAILED };

struct ThreadResult {
  size_t bytes_sent = 0;
  size_t success_count = 0;
  size_t error_count = 0;
  double total_latency_ms = 0;
  std::map<ErrorType, size_t> error_details;

  void add_error(ErrorType type) {
    error_count++;
    error_details[type]++;
  }
};

void worker_thread(size_t payload_size, ThreadResult& res) {
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) return;

  int flag = 1;
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, (char*)&flag, sizeof(int)) < 0) {
    static bool warned = false;
    if (!warned) {
      std::cerr << "Failed to set TCP_NODELAY" << std::endl;
      warned = true;
    }
  }

  struct timeval tv;
  tv.tv_sec = 2;
  tv.tv_usec = 0;
  setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

  sockaddr_in addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(SERVER_PORT);
  inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

  if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
    res.add_error(ErrorType::CONNECTION_LOST);
    close(sock);
    return;
  }

  const size_t buf_capacity = SEND_CHUNK_SIZE + 64;
  std::vector<char> buffer(buf_capacity, 'x');
  char recv_buf[1024];
  int iter = 0;

  while (keep_running.load(std::memory_order_relaxed)) {
    int h_len = snprintf(buffer.data(), 64, "ID:%d:", iter++);
    size_t total_to_send = h_len + payload_size;

    auto start = std::chrono::high_resolution_clock::now();
    size_t total_sent = 0;
    bool comm_error = false;

    while (total_sent < total_to_send) {
      size_t remaining = total_to_send - total_sent;
      size_t current_chunk = std::min(remaining, SEND_CHUNK_SIZE);

      if (remaining <= SEND_CHUNK_SIZE) {
        buffer[current_chunk - 1] = '\n';
      }

      ssize_t s_res = send(sock, buffer.data(), current_chunk, 0);
      if (s_res <= 0) {
        res.add_error(ErrorType::SEND_FAILED);
        comm_error = true;
        break;
      }
      total_sent += s_res;

      if (remaining <= SEND_CHUNK_SIZE) {
        buffer[current_chunk - 1] = 'x';
      }
    }

    if (comm_error) break;

    ssize_t r_res = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
    auto end = std::chrono::high_resolution_clock::now();

    if (r_res > 0) {
      if (recv_buf[r_res - 1] == '\n') {
        res.success_count++;
        res.bytes_sent += total_sent;
        res.total_latency_ms += std::chrono::duration<double, std::milli>(end - start).count();
      } else {
        res.add_error(ErrorType::INVALID_FORMAT);
      }
    } else {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        res.add_error(ErrorType::TIMEOUT);
      } else {
        res.add_error(ErrorType::CONNECTION_LOST);
        break;
      }
    }
  }

  shutdown(sock, SHUT_WR);
  while (recv(sock, recv_buf, sizeof(recv_buf), 0) > 0);
  close(sock);
}

std::string error_to_string(ErrorType type) {
  switch (type) {
    case ErrorType::CONNECTION_LOST:
      return "Conn Lost";
    case ErrorType::TIMEOUT:
      return "Timeout";
    case ErrorType::INVALID_FORMAT:
      return "Bad Format";
    case ErrorType::SEND_FAILED:
      return "Send Fail";
    default:
      return "Unknown";
  }
}

void run_test_case(int num_threads, size_t packet_size) {
  keep_running = true;
  std::vector<ThreadResult> results(num_threads);
  std::vector<std::thread> threads;

  auto start_time = std::chrono::high_resolution_clock::now();
  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(worker_thread, packet_size, std::ref(results[i]));
  }

  std::this_thread::sleep_for(std::chrono::seconds(TEST_DURATION_SEC));
  keep_running = false;

  for (auto& t : threads) t.join();
  auto end_time = std::chrono::high_resolution_clock::now();

  size_t total_ok = 0, total_err = 0, total_bytes = 0;
  double aggregate_lat = 0;
  std::map<ErrorType, size_t> total_error_details;

  for (const auto& r : results) {
    total_ok += r.success_count;
    total_err += r.error_count;
    total_bytes += r.bytes_sent;
    aggregate_lat += r.total_latency_ms;
    for (auto const& [type, count] : r.error_details) {
      total_error_details[type] += count;
    }
  }

  double duration = std::chrono::duration<double>(end_time - start_time).count();
  double rps = total_ok / duration;
  double mbps = (static_cast<double>(total_bytes) / (1024.0 * 1024.0)) / duration;
  double avg_lat = total_ok > 0 ? aggregate_lat / total_ok : 0;

  // Print main metrics
  std::cout << "| " << std::setw(12) << packet_size << " | " << std::setw(8) << total_ok << " | " << std::setw(8)
            << total_err << " | " << std::setw(8) << (int)rps << " | " << std::setw(10) << std::fixed
            << std::setprecision(2) << mbps << " | " << std::setw(9) << std::fixed << std::setprecision(3) << avg_lat
            << " |";

  // Print error summary if any
  if (total_err > 0) {
    std::cout << " [Errors: ";
    for (auto const& [type, count] : total_error_details) {
      std::cout << error_to_string(type) << ":" << count << " ";
    }
    std::cout << "]";
  }
  std::cout << std::endl;
}

int main() {
  int num_cores = std::thread::hardware_concurrency();
  if (num_cores == 0) num_cores = 1;

  std::cout << "Starting Advanced Performance Matrix" << std::endl;
  std::cout << "Fixed Threads (Cores): " << num_cores << std::endl;
  std::cout << "Duration: " << TEST_DURATION_SEC << "s per test" << std::endl;

  std::cout << std::string(85, '-') << std::endl;
  std::cout << "| Payload (B)  |   OK     | Failed   |  Req/s   |   MB/s     | Lat (ms)  |" << std::endl;
  std::cout << std::string(85, '-') << std::endl;

  for (size_t size : PACKET_SIZES) {
    run_test_case(num_cores, size);
  }

  std::cout << std::string(85, '-') << std::endl;
  return 0;
}