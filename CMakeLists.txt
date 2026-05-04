cmake_minimum_required(VERSION 3.22)
project(HashTcpServer VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(GTest REQUIRED)
find_package(spdlog REQUIRED)

add_library(server_core STATIC
    src/network/tcp_server.cpp
    src/network/connection.cpp
    src/hash/hash_engine.cpp
    src/utils/arg_parser.cpp
    src/utils/logger.cpp
)

target_include_directories(server_core PUBLIC src)
target_link_libraries(server_core PUBLIC Boost::boost OpenSSL::Crypto Threads::Threads spdlog::spdlog)

add_executable(hash_server src/main.cpp)
target_link_libraries(hash_server PRIVATE server_core)

enable_testing()
add_executable(unit_tests
    tests/unit/arg_parser_test.cpp
    tests/unit/hash_engine_test.cpp
    tests/unit/network_test.cpp
)
target_link_libraries(unit_tests PRIVATE server_core GTest::gtest GTest::gtest_main)
add_test(NAME AllUnitTests COMMAND unit_tests)

add_executable(server_benchmark tests/benchmark/server_benchmark.cpp)
target_link_libraries(server_benchmark PRIVATE Boost::boost)
