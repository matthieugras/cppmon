#ifndef UTIL_H
#define UTIL_H

#include <boost/hana.hpp>

namespace detail {
namespace hana = boost::hana;
template<typename>
inline constexpr bool always_false_v = false;
template<typename T, typename... Os>
inline constexpr auto any_type_equal() {
  auto r = hana::any_of(hana::tuple_t<Os...>,
                        [](auto t) { return t == hana::type_c<T>; });
  return r;
}
template<typename T, typename... Os>
inline constexpr bool any_type_equal_v =
        decltype(any_type_equal<T, Os...>())::value;

template<typename S>
bool is_subset(const S &set1, const S &set2) {
  for (const auto &elem : set1) {
    if (!set2.contains(elem))
      return false;
  }
  return true;
}
template<class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
}// namespace detail

using detail::always_false_v;
using detail::any_type_equal_v;
using detail::is_subset;
using detail::overloaded;

#endif
