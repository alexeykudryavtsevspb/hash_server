# High-Performance TCP Hash Server

A streaming-capable C++ TCP server designed to calculate cryptographic hash sums of arbitrary-length input with predictable memory usage and high concurrency.


## Key Features

*   **High Concurrency & Resilience**: Utilizes an asynchronous event-driven architecture with a self-healing thread pool. Worker threads automatically recover from unhandled exceptions, maintaining full server capacity at all times.
*   **Memory-Efficient Streaming**: Implements a zero-copy ingestion path using `std::string_view` and direct buffer passing to OpenSSL EVP. Processes multi-gigabyte data streams with a predictable memory footprint of ~64KB per session.
*   **Lock-Free Scalability**: Leverages `boost::asio::strand` for logical command serialization. This eliminates expensive OS-level mutexes and prevents thread contention while ensuring data integrity.
*   **Resource Lifecycle Control**: Features automated pruning of idle connections via watchdog timers (30s timeout) and a robust signal-handling system for graceful shutdowns.


## Quick Start

For project evaluation, you only need **Docker** and **Ubuntu 24.04** (or any Linux distribution with `make`). The Docker build process is self-contained: it automatically installs all dependencies, compiles the code, and **runs the full test suite** (GTest + Pytest) during the build stage.

### 1. Install the Docker

```bash
sudo apt update && sudo apt install -y docker.io
```

### 2. Build and Test
This command builds a multi-stage production image. If any test fails, the build will terminate.
```bash
make docker_build
```

### 3. Run the Server
Once built, launch the containerized server (defaulting to SHA-256 on port 12345):
```bash
make docker_run
```

### 4. Verify
In a separate terminal, test the running container:
```bash
echo "hello" | nc -N localhost 12345
```
Expected output: `2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824`


## Local Development (Optional)

If you intend to modify the code or run benchmarks locally, you will need to install the development environment.
The project is optimized for **Ubuntu 24.04**.

### 1. Install Dependencies
```bash
sudo apt update && sudo apt install -y \
    build-essential \
    cmake \
    clang-format \
    docker.io \
    libboost-all-dev \
    libssl-dev \
    libspdlog-dev \
    googletest \
    python3.12-venv
```

### 2. Standard Build
```bash
make build
```
### 3. Running the Server
Usage: `./hash_server [host:port] [--algo sha256|md5|sha512] [-v]`
```bash
./build/hash_server 0.0.0.0:12345 --algo sha256 --verbose
```


## Testing & Benchmarking

### 1. Unit & Integration Tests
Runs both Unit (GTest) and Integration tests (Pytest).
```bash
make all_test
```

### 2. Manual Verification
You can verify the streaming and hashing capability using `netcat`:
```bash
echo "hello" | nc -N localhost 12345
```
Expected output: `2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824`

### 3. Performance Benchmark
The benchmark tool measures throughput for various packet sizes, from 64 bytes up to 1GB.
```bash
make benchmark
```

## Project Structure
*   `doc/`: Documents
*   `src/network/`: Asynchronous TCP acceptor and connection handling.
*   `src/hash/`: Wrapper around OpenSSL EVP for streaming hashes.
*   `src/utils/`: High-performance logging and CLI argument parsing.
*   `tests/`: Unit (GTest), Integration (Pytest), and Benchmarks.


## Engineering Trade-offs

### 1. Asynchronous Architecture & Thread Resilience
- **Decision**: Employed `Boost.Asio` with a fixed thread pool and a "Keep-Alive" restart loop.
- **Justification**: A "thread-per-connection" model scales poorly due to context-switching and stack memory overhead. Our async model handles thousands of concurrent sessions with a minimal thread footprint (matching `hardware_concurrency`).
- **Resilience**: Unlike standard implementations where an unhandled exception might kill a worker thread, our server catches errors and restarts the `io_context::run` loop. This prevents "thread bleeding" and maintains full processing capacity after partial failures.
- **Trade-off**: Requires strict object lifetime management using `std::shared_from_this` to prevent use-after-free during pending callbacks.

### 2. Memory Efficiency: Streaming vs. Buffer Bloat
- **Decision**: Incremental hashing via `EVP_DigestUpdate` with a fixed 64KB buffer.
- **Justification**: Most implementations buffer entire lines, making them vulnerable to Out-Of-Memory (OOM) crashes if a client sends multi-gigabyte data without a newline. Our streaming approach processes data in constant-sized chunks.
- **Trade-off**: We maintain a unique `EVP_MD_CTX` state per client to support multi-read messages, which slightly increases the memory footprint for idle connections to ensure correctness for large payloads.

### 3. Synchronization: Strands vs. OS Mutexes
- **Decision**: Utilized `boost::asio::strand` for task serialization.
- **Justification**: To utilize multiple cores, we run the `io_context` in a pool. However, a single connection's state (hash context and write queue) must be modified sequentially. Strands provide "logical" serialization without the overhead of heavy OS-level mutexes or the risk of deadlocks.
- **Benefit**: Maximizes L1/L2 cache locality by keeping session processing within a consistent execution context.

### 4. Network: Latency vs. Throughput (TCP_NODELAY)
- **Decision**: Disabled Nagle’s Algorithm (`tcp::no_delay(true)`).
- **Justification**: Since the server is request-response oriented (calculating a hash and returning it immediately), we prioritize latency over bandwidth efficiency. Sending the 64-character hash as soon as it's ready provides a more responsive experience.
- **Trade-off**: Slightly increased network packet overhead due to more frequent, smaller TCP segments.

### 5. Production Readiness & Risk Mitigation
- **Write Queue (Backpressure)**: Currently uses an unbounded `std::deque` for simplicity. In a production environment, this could lead to memory exhaustion if a client uploads faster than it can download the results. A production-ready fix would involve suspending `async_read` when the queue size hits a "high-water mark".
- **DoS Protection**: The server lacks global or per-IP connection limits to prioritize code clarity for this demonstration. It is vulnerable to File Descriptor exhaustion. Production deployments should include a `connection_manager` to cap active sessions.
- **Input Constraints**: While the streaming model handles any length, a "Slowloris" attack (sending data without a newline forever) could keep hash contexts open indefinitely. A production system would enforce a maximum line length or a global session timeout.


## Performance Observations & Hypotheses

```
-------------------------------------------------------------------------------------
| Payload (B)  |   OK     | Failed   |  Req/s   |   MB/s     | Lat (ms)  |
-------------------------------------------------------------------------------------
|           64 |   419696 |        0 |    83904 |       5.82 |     0.141 |
|         1024 |   420276 |        0 |    84038 |      82.76 |     0.141 |
|         2048 |   417769 |        0 |    83533 |     163.84 |     0.142 |
|         4096 |   368921 |        0 |    73768 |     288.77 |     0.161 |
|        16384 |   270165 |        0 |    54020 |     844.50 |     0.220 |
|        65536 |   130634 |        0 |    26119 |    1632.66 |     0.457 |
|       131072 |    77484 |        0 |    15490 |    1936.48 |     0.772 |
|      1048576 |    12697 |        0 |     2536 |    2536.85 |     4.724 |
|   1073741824 |       24 |        0 |        4 |    4614.94 |  2602.025 |
-------------------------------------------------------------------------------------
```

The following observations were made during initial benchmarking on Ubuntu 24.04. These are presented as technical hypotheses based on observed metrics rather than exhaustive profiling.

### 1. Latency Stability (64B – 4KB)
- **Observation**: Latency remains exceptionally flat (~0.14ms) for small to medium payloads.
- **Hypothesis**: This confirms the efficacy of `tcp::no_delay(true)` in bypassing Nagle’s algorithm. Avoiding kernel-level buffering allows for predictable response times in request-heavy workloads.

### 2. Cache Residency & Scaling (16KB – 128KB)
- **Observation**: Throughput increases linearly as payload size grows.
- **Hypothesis**: The 64KB internal buffer size aligns with typical CPU cache architectures. This minimizes L2/L3 cache misses during `EVP_DigestUpdate` calls, allowing the CPU to process data as fast as the I/O subsystem delivers it.

### 3. Throughput Peak (1GB+)
- **Observation**: Throughput saturates at approximately **4.6 GB/s** for SHA-256.
- **Hypothesis**: At this volume, the system becomes CPU-bound due to the cryptographic overhead of hashing. Because memory usage remains constant, the bottleneck is purely the raw computational cycles of the OpenSSL engine and the available memory bandwidth.