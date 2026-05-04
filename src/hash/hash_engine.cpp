#include "hash/hash_engine.h"

#include <iomanip>
#include <stdexcept>

HashEngine::HashEngine(HashAlgorithm algo) {
  if (!md_ctx_) throw std::runtime_error("Failed to create OpenSSL context");

  switch (algo) {
    case HashAlgorithm::MD5:
      md_type_ = EVP_md5();
      break;
    case HashAlgorithm::SHA256:
      md_type_ = EVP_sha256();
      break;
    case HashAlgorithm::SHA512:
      md_type_ = EVP_sha512();
      break;
    default:
      throw std::invalid_argument("Unknown hash algorithm");
  }

  init_context();
}

void HashEngine::init_context() {
  if (EVP_DigestInit_ex(md_ctx_.get(), md_type_, nullptr) != 1) {
    throw std::runtime_error("Failed to initialize digest");
  }
}

void HashEngine::update(const void* data, size_t len) {
  if (EVP_DigestUpdate(md_ctx_.get(), data, len) != 1) {
    throw std::runtime_error("Failed to update digest");
  }
}

std::string HashEngine::finalize() {
  unsigned char hash[EVP_MAX_MD_SIZE];
  unsigned int length = 0;

  if (EVP_DigestFinal_ex(md_ctx_.get(), hash, &length) != 1) {
    throw std::runtime_error("Failed to finalize digest");
  }

  init_context();

  std::string hex_hash;
  hex_hash.resize(length * 2);
  static const char hex_chars[] = "0123456789abcdef";
  for (unsigned int i = 0; i < length; ++i) {
    hex_hash[i * 2] = hex_chars[(hash[i] >> 4) & 0x0F];
    hex_hash[i * 2 + 1] = hex_chars[hash[i] & 0x0F];
  }

  return hex_hash;
}