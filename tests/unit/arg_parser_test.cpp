#include "utils/arg_parser.h"
#include <gtest/gtest.h>

using namespace utils;

TEST(ArgParserTest, SuccessOnDefault) {
  char* argv[] = {(char*)"hash_server"};
  auto result = parse_arguments(1, argv);

  ASSERT_EQ(result.status, ParseResult::Status::Success);
  ASSERT_TRUE(result.config.has_value());
  EXPECT_EQ(result.config->host, "0.0.0.0");
  EXPECT_EQ(result.config->port, 12345);
}

TEST(ArgParserTest, SuccessOnCustomAddr) {
  char* argv[] = {(char*)"hash_server", (char*)"0.0.0.0:9999"};
  auto result = parse_arguments(2, argv);

  ASSERT_EQ(result.status, ParseResult::Status::Success);
  EXPECT_EQ(result.config->host, "0.0.0.0");
  EXPECT_EQ(result.config->port, 9999);
}

TEST(ArgParserTest, HandleHelpFlag) {
  char* argv[] = {(char*)"hash_server", (char*)"--help"};
  auto result = parse_arguments(2, argv);

  EXPECT_EQ(result.status, ParseResult::Status::Help);
  EXPECT_EQ(result.exit_code, 0);
  EXPECT_FALSE(result.config.has_value());
}

TEST(ArgParserTest, ErrorOnInvalidAlgo) {
  char* argv[] = {(char*)"hash_server", (char*)"--algo", (char*)"invalid_hash"};
  auto result = parse_arguments(3, argv);

  EXPECT_EQ(result.status, ParseResult::Status::Error);
  EXPECT_EQ(result.exit_code, 2);
  EXPECT_FALSE(result.config.has_value());
}

TEST(ArgParserTest, ErrorOnInvalidPort) {
  char* argv[] = {(char*)"hash_server", (char*)"0.0.0.0:70000"};
  auto result = parse_arguments(2, argv);

  EXPECT_EQ(result.status, ParseResult::Status::Error);
  EXPECT_EQ(result.exit_code, 2);
}

TEST(ArgParserTest, VerboseFlagParsing) {
  char* argv[] = {(char*)"hash_server", (char*)"-v", (char*)"localhost:80"};
  auto result = parse_arguments(3, argv);

  ASSERT_EQ(result.status, ParseResult::Status::Success);
  EXPECT_TRUE(result.config->verbose);
  EXPECT_EQ(result.config->host, "localhost");
  EXPECT_EQ(result.config->port, 80);
}

TEST(ArgParserTest, CaseInsensitiveAlgo) {
  char* argv[] = {(char*)"hash_server", (char*)"-a", (char*)"MD5"};
  auto result = parse_arguments(3, argv);

  ASSERT_EQ(result.status, ParseResult::Status::Success);
  EXPECT_EQ(result.config->algo, HashAlgorithm::MD5);
}
