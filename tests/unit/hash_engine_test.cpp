#include "hash/hash_engine.h"

#include <gtest/gtest.h>

// --- Basic functional tests for all supported algorithms using "hello"

TEST(HashEngineTest, HelloMD5) {
  HashEngine engine(HashAlgorithm::MD5);
  engine.update("hello", 5);
  // Expected MD5 for "hello"
  EXPECT_EQ(engine.finalize(), "5d41402abc4b2a76b9719d911017c592");
}

TEST(HashEngineTest, HelloSHA256) {
  HashEngine engine(HashAlgorithm::SHA256);
  engine.update("hello", 5);
  // Expected SHA-256 for "hello"
  EXPECT_EQ(engine.finalize(), "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

TEST(HashEngineTest, HelloSHA512) {
  HashEngine engine(HashAlgorithm::SHA512);
  engine.update("hello", 5);
  // Expected SHA-512 for "hello"
  const std::string expected =
      "9b71d224bd62f3785d96d46ad3ea3d73319bfbc2890caadae2dff72519673ca72323c3d99ba5c11d7c7acc6e14b8c5da0c4663475c2e5c3a"
      "def46f73bcdec043";
  EXPECT_EQ(engine.finalize(), expected);
}

// --- Extended tests for the default SHA256 algorithm

TEST(HashEngineTest, SHA256EmptyString) {
  HashEngine engine(HashAlgorithm::SHA256);
  // Finalizing without any update() calls to test empty input handling
  // Expected SHA-256 for ""
  EXPECT_EQ(engine.finalize(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(HashEngineTest, SHA256ExplicitEmptyUpdate) {
  HashEngine engine(HashAlgorithm::SHA256);

  // Explicitly update with 0 bytes of data
  engine.update("", 0);

  // Expected SHA-256 for an empty string
  EXPECT_EQ(engine.finalize(), "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(HashEngineTest, SHA256SplitUpdate) {
  HashEngine engine(HashAlgorithm::SHA256);

  // Simulate receiving data in two separate TCP chunks
  engine.update("hel", 3);
  engine.update("lo", 2);

  // The result must be identical to hashing "hello" in a single block
  EXPECT_EQ(engine.finalize(), "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}
