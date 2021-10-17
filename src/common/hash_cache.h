#ifndef CPPMON_HASH_CACHE_H
#define CPPMON_HASH_CACHE_H
#include <absl/hash/hash.h>
#include <cstddef>
#include <fmt/core.h>
#include <fmt/format.h>

namespace common {
template<typename T>
class hash_cached {
  friend ::fmt::formatter<hash_cached<T>>;

public:
  hash_cached() : key_(T()), hash_(absl::Hash<T>()(key_)) {}

  hash_cached(const T &key) : key_(key), hash_(absl::Hash<T>()(key_)) {}
  hash_cached(T &&key) : key_(std::move(key)), hash_(absl::Hash<T>()(key_)) {}

  T const &key() const { return key_; }

  hash_cached(hash_cached<T> &&) noexcept = default;
  hash_cached(hash_cached<T> const &) = default;
  hash_cached<T> &operator=(hash_cached<T> &&) = default;
  hash_cached<T> &operator=(hash_cached<T> const &) = default;

  T move_key_out() {
    return std::move(key_);
  }

  friend bool operator==(const hash_cached<T> &lhs, const hash_cached<T> &rhs) {
    return lhs.key_ == rhs.key_;
  }

  template<typename H>
  friend H AbslHashValue(H h, const hash_cached<T> &elem) {
    return H::combine(std::move(h), elem.hash_);
  }

private:
  T key_;
  size_t hash_;
};
}// namespace common

template<typename T>
struct [[maybe_unused]] fmt::formatter<common::hash_cached<T>> {
  constexpr auto parse [[maybe_unused]] (format_parse_context &ctx)
  -> decltype(auto) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}')
      throw format_error("invalid format - only empty format strings are "
                         "accepted for hash_cached");
    return it;
  }

  template<typename FormatContext>
  auto format
    [[maybe_unused]] (const common::hash_cached<T> &val, FormatContext &ctx)
    -> decltype(ctx.out()) {
    return format_to(ctx.out(), "{}", val.key_);
  }
};


#endif
