# Technical Specification: High-Performance TCP Hash Server

## Objective

Implement a high-performance TCP server in C++ capable of calculating hash sums for arbitrary-length input strings.

## Task Description

Develop a console-based C++ application that acts as a TCP server. The server should accept client connections, receive newline-terminated strings, compute their hash, and return the result in hexadecimal format.

## Application Requirements

### Execution

The server must run as a command-line application:
```
<server> <host:port>
    <host:port> — address and port to bind the server (default: 0.0.0.0:12345)
```

### Startup Behavior

- The server starts listening for incoming TCP connections on the specified address.
- It must support multiple simultaneous client connections.

### Core Functionality

#### Communication Protocol

- Clients connect via TCP (e.g., using telnet or netcat).
- Each client sends one or more messages:
    - Messages are UTF-8 strings terminated by a newline character \n (byte value 0x0A).
- The server processes input as a stream and must correctly handle partial reads and message boundaries.

#### Request Handling

For every complete input line received:
1.  Compute a hash of the string (algorithm of your choice, e.g., SHA-256, MD5, or similar).
2.  Encode the hash as a hexadecimal string.
3.  Send the result back to the client, terminated by \n.

Example:
```
Client: hello\n
Server: 2cf24dba5fb0a...<hex>\n
```

### Concurrency & Performance

- The server must handle multiple clients concurrently.
- The design should utilize available CPU cores efficiently (e.g., thread pool, async I/O, or event-driven model).
- Avoid blocking operations that degrade scalability.

### Streaming & Memory Efficiency

- Input strings must not be limited in length.
- The server must process data in a streaming fashion:
    - Avoid loading entire large inputs into memory when possible.
    - Minimize unnecessary memory copies and redundant transformations.
- Responses should be sent as soon as they are ready (no batching delays).

### Error Handling

Gracefully handle:
- Client disconnects
- Malformed input (e.g., missing newline)
- Internal processing errors
- The server should remain stable under faulty or malicious input.

### Technical Constraints

- Must compile and run on Ubuntu 22.04 or 24.04 using standard packages.
- Any C++ standard (C++20 or newer recommended), compiler, or build system may be used.
- External libraries are allowed if available via standard package managers.

### Design Expectations

Focus on simplicity and clarity rather than production-grade completeness.

Emphasis should be on:

- Clean, modular architecture
- Readable and maintainable code
- Efficient resource usage (CPU and memory)
- Proper separation of concerns (networking, parsing, hashing, etc.)

## Testing Requirements

- All major modules must have unit test coverage.
- Tests may be implemented in C++, Python, or another suitable language.
- Include functional or integration tests that validate:

- End-to-end client-server interaction
- Handling of multiple concurrent clients

## Deliverables

- Source code hosted on a public repository (e.g., GitHub)
- Build configuration (e.g., CMake)
- README file including:
    - Project overview
    - Build instructions
    - Run instructions
    - Example usage (e.g., with netcat)

## Bonus (Optional Enhancements)

- Support for configurable hash algorithms
- Logging of client requests and server activity
- Benchmarking or performance measurements
- Dockerfile for build and execution
- Load testing scripts

## Notes

- If any part of the specification is unclear, make reasonable assumptions and document them.
- Focus on demonstrating engineering judgment and trade-offs.
