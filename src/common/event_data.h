#ifndef EVENT_DATA_H
#define EVENT_DATA_H

#include <absl/base/optimization.h>
#include <boost/operators.hpp>
#include <boost/variant2/variant.hpp>
#include <fmt/core.h>
#include <fmt/format.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <util.h>

namespace common {
namespace {
  namespace var2 = boost::variant2;
  using nlohmann::json;
  using var2::variant;
}// namespace

class event_data : boost::equality_comparable<event_data> {
  friend ::fmt::formatter<event_data>;
  friend struct compat_less;

public:
  event_data();
  event_data &operator=(event_data &&other) noexcept;
  event_data &operator=(const event_data &other);
  event_data(const event_data &other);
  event_data(event_data &&other) noexcept;
  ~event_data();

  static event_data Int(int64_t i);
  static event_data Float(double d);
  static event_data String(const std::string &s);
  static event_data String(std::string &&s);
  static event_data from_json(const json &json_formula);
  static event_data nan();

  double to_double() const;
  bool operator==(const event_data &other) const;
  bool operator<=(const event_data &other) const;
  bool operator<(const event_data &other) const;
  bool operator>(const event_data &other) const;
  event_data operator+(const event_data &other) const;
  event_data operator-(const event_data &other) const;
  event_data operator-() const;
  event_data operator*(const event_data &other) const;
  event_data operator/(const event_data &other) const;
  event_data operator%(const event_data &other) const;
  [[nodiscard]] event_data int_to_float() const;
  [[nodiscard]] event_data float_to_int() const;
  [[nodiscard]] const int64_t *get_if_int() const;

  template<typename H>
  friend H AbslHashValue(H h, const event_data &elem) {
    if (elem.tag == HOLDS_INT) {
      return H::combine(std::move(h), elem.i);
    } else if (elem.tag == HOLDS_FLOAT) {
      return H::combine(std::move(h), elem.d);
    } else {
      return H::combine(std::move(h), *elem.s);
    }
  }

private:
  void do_move(event_data &&other) noexcept;
  void do_copy(const event_data &other);
  template<typename F>
  static event_data apply_arith_bin_op(F op, const event_data &l,
                                       const event_data &r) {
    if (l.tag == HOLDS_INT && r.tag == HOLDS_INT) {
      return Int(op(l.i, r.i));
    } else if (l.tag == HOLDS_INT && r.tag == HOLDS_FLOAT) {
      return Float(op(static_cast<double>(l.i), r.d));
    } else if (l.tag == HOLDS_FLOAT && r.tag == HOLDS_INT) {
      return Float(op(l.d, static_cast<double>(r.i)));
    } else if (l.tag == HOLDS_FLOAT && r.tag == HOLDS_FLOAT) {
      return Float(op(l.d, r.d));
    } else {
      return nan();
    }
  }
  enum tag_ty
  {
    HOLDS_INT,
    HOLDS_FLOAT,
    HOLD_STRING
  } tag;
  union {
    int64_t i;
    double d;
    std::string *s;
  };
};

struct compat_less {
  bool operator()(const event_data &a, const event_data &b) {
    if (a.tag == event_data::HOLDS_FLOAT && b.tag == event_data::HOLDS_FLOAT) {
      if (a.d != a.d) {
        // a is NaN, hence a is less than b unless b is NaN
        return b.d == b.d;
      } else if (b.d != b.d) {
        // b is NaN but a is not, hence b is less
        return false;
      } else {
        return a.d < b.d;
      }
    } else {
      return a < b;
    }
  }
};

}// namespace common

template<>
struct [[maybe_unused]] fmt::formatter<common::event_data> {
  constexpr auto parse [[maybe_unused]] (format_parse_context &ctx)
  -> decltype(auto) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}')
      throw format_error("invalid format - only empty format strings are "
                         "accepted for event_data");
    return it;
  }

  template<typename FormatContext>
  auto format
    [[maybe_unused]] (const common::event_data &tab, FormatContext &ctx) const
    -> decltype(ctx.out()) {
    if (tab.tag == common::event_data::HOLDS_INT) {
      return format_to(ctx.out(), "{}", tab.i);
    } else if (tab.tag == common::event_data::HOLDS_FLOAT) {
      return format_to(ctx.out(), "{:.15e}", tab.d);
    } else {
      return format_to(ctx.out(), "\"{}\"", *tab.s);
    }
  }
};
#endif
