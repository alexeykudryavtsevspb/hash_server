#pragma once

#include "hash/hash_algorithm.h"

#include <openssl/evp.h>

#include <memory>
#include <string>

class HashEngine {
 public:
  explicit HashEngine(HashAlgorithm algo);
  ~HashEngine() = default;

  HashEngine(const HashEngine&) = delete;
  HashEngine& operator=(const HashEngine&) = delete;

  void update(const void* data, size_t len);
  std::string finalize();

 private:
  void init_context();

  const EVP_MD* md_type_{};

  struct EvpCtxDeleter {
    void operator()(EVP_MD_CTX* ctx) const {
      if (ctx) EVP_MD_CTX_free(ctx);
    }
  };
  std::unique_ptr<EVP_MD_CTX, EvpCtxDeleter> md_ctx_{EVP_MD_CTX_new()};
};