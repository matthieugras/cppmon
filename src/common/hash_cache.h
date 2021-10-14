#ifndef CPPMON_HASH_CACHE_H
#define CPPMON_HASH_CACHE_H
#include <absl/hash/hash.h>
#include <cstddef>

namespace common {
template<typename T>
class hash_cached {
public:
  hash_cached() : key_(T()), hash_(absl::Hash<T>(key_)) {}

  hash_cached(const T &key) : key_(key), hash_(absl::Hash<T>(key_)) {}
  hash_cached(T &&key) : key_(std::move(key)), hash_(absl::Hash<T>(key_)) {}

  hash_cached(hash_cached<T> &&) noexcept = default;
  hash_cached(hash_cached<T> const &) = default;
  hash_cached<T> &operator=(hash_cached<T> &&) = default;
  hash_cached<T> &operator=(hash_cached<T> const &) = default;

  friend bool operator==(const hash_cached<T> &lhs, const hash_cached<T> &rhs) {
    return lhs.key_ == rhs.key_;
  }

  template<typename H>
  friend H AbslHashValue(H h, const hash_cached<T> &elem) {
    return h;
  }


private:
  T key_;
  size_t hash_;
};
}// namespace common

#endif
