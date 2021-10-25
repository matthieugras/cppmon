#ifndef UTIL_H
#define UTIL_H

#include <algorithm>
#include <boost/hana.hpp>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace detail {

class not_implemented_error : public std::domain_error {
public:
  not_implemented_error() : std::domain_error("not implemented.") {}
};

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
  return std::all_of(set1.begin(), set1.end(),
                     [&set2](const auto &elem) { return set2.contains(elem); });
}

template<typename S, typename... Rest>
S hash_set_union(const S &s, const Rest &...rest) {
  S ret_set = s;
  auto sets_as_tup = hana::make_tuple(rest...);
  hana::for_each(sets_as_tup, [&ret_set](const auto &s) {
    ret_set.insert(s.begin(), s.end());
  });
  return ret_set;
}

template<class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;
}// namespace detail

std::string read_file(const std::filesystem::path &path);

using detail::always_false_v;
using detail::any_type_equal_v;
using detail::hash_set_union;
using detail::is_subset;
using detail::not_implemented_error;
using detail::overloaded;

#endif
