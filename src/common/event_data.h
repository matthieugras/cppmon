#ifndef EVENT_DATA_H
#define EVENT_DATA_H

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

class event_data_opt : boost::equality_comparable<event_data_opt> {
  friend ::fmt::formatter<event_data_opt>;

public:
  event_data_opt() = default;

  static event_data_opt Int(int i) {
    event_data_opt tmp;
    tmp.tag = HOLDS_INT;
    tmp.i = i;
    return tmp;
  }
  static event_data_opt Float(double d) {
    event_data_opt tmp;
    tmp.tag = HOLDS_FLOAT;
    tmp.d = d;
    return tmp;
  }
  static event_data_opt String(const std::string &s) {
    event_data_opt tmp;
    tmp.tag = HOLD_STRING;
    tmp.s = new std::string(s);
    return tmp;
  }
  static event_data_opt String(std::string &&s) {
    event_data_opt tmp;
    tmp.tag = HOLD_STRING;
    tmp.s = new std::string(std::move(s));
    return tmp;
  }
  static event_data_opt from_json(const json &json_formula);
  static event_data_opt nan();

  bool operator==(const event_data_opt &other) const;
  bool operator<=(const event_data_opt &other) const;
  bool operator<(const event_data_opt &other) const;
  event_data_opt operator+(const event_data_opt &other) const;
  event_data_opt operator-(const event_data_opt &other) const;
  event_data_opt operator-() const;
  event_data_opt operator*(const event_data_opt &other) const;
  event_data_opt operator/(const event_data_opt &other) const;
  event_data_opt operator%(const event_data_opt &other) const;
  [[nodiscard]] event_data_opt int_to_float() const;
  [[nodiscard]] event_data_opt float_to_int() const;
  [[nodiscard]] const int *get_if_int() const;

  event_data_opt(const event_data_opt &other) : tag(other.tag) {
    if (other.tag == HOLDS_INT) {
      i = other.i;
    } else if (other.tag == HOLDS_FLOAT) {
      d = other.d;
    } else {
      if (tag == HOLD_STRING)
        *s = *other.s;
      else
        s = new std::string(*other.s);
    }
  }
  event_data_opt(event_data_opt &&other) noexcept : tag(other.tag) {
    if (other.tag == HOLDS_INT) {
      i = other.i;
    } else if (other.tag == HOLDS_FLOAT) {
      d = other.d;
    } else {
      s = other.s;
      other.s = nullptr;
    }
  }
  ~event_data_opt() {
    if (tag == HOLD_STRING) {
      delete s;
    }
  }

  template<typename H>
  friend H AbslHashValue(H h, const event_data_opt &elem) {
    if (elem.tag == HOLDS_INT) {
      return H::combine(std::move(h), elem.i);
    } else if (elem.tag == HOLDS_FLOAT) {
      return H::combine(std::move(h), elem.d);
    } else {
      return H::combine(std::move(h), *elem.s);
    }
  }

  friend void swap(event_data_opt &l, event_data_opt &r) noexcept {
    using std::swap;
    swap(l, r);
  }

private:
  template<typename F>
  static event_data_opt apply_arith_bin_op(F op, const event_data_opt &l,
                                           const event_data_opt &r) {
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
    int i;
    double d;
    std::string *s;
  };
};

class event_data : boost::equality_comparable<event_data> {
  friend ::fmt::formatter<event_data>;

public:
  bool operator==(const event_data &other) const;
  bool operator<=(const event_data &other) const;
  bool operator<(const event_data &other) const;
  event_data operator+(const event_data &other) const;
  event_data operator-(const event_data &other) const;
  event_data operator-() const;
  event_data operator*(const event_data &other) const;
  event_data operator/(const event_data &other) const;
  event_data operator%(const event_data &other) const;
  [[nodiscard]] event_data int_to_float() const;
  [[nodiscard]] event_data float_to_int() const;
  [[nodiscard]] const int *get_if_int() const;

  template<typename H>
  friend H AbslHashValue(H h, const event_data &d) {
    if (const int *iptr = var2::get_if<int>(&d.val)) {
      return H::combine(std::move(h), *iptr);
    } else if (const double *dptr = var2::get_if<double>(&d.val)) {
      return H::combine(std::move(h), *dptr);
    } else {
      const std::string *sptr = var2::get_if<std::string>(&d.val);
      return H::combine(std::move(h), *sptr);
    }
  }

  friend void swap(event_data &l, event_data &r) noexcept {
    using std::swap;
    swap(l.val, r.val);
  }

  static event_data nan();
  static event_data Int(int i);
  static event_data Float(double d);
  static event_data String(std::string s);
  static event_data from_json(const json &json_formula);

private:
  using val_type = variant<int, double, std::string>;
  explicit event_data(val_type &&val) noexcept;
  val_type val;

  template<typename F>
  static event_data apply_arith_bin_op(F op, const event_data &l,
                                       const event_data &r) {
    using std::is_same_v;
    auto visitor = [op](auto &&l, auto &&r) {
      using TL = std::decay_t<decltype(l)>;
      using TR = std::decay_t<decltype(r)>;

      if constexpr (is_same_v<TL, int> && is_same_v<TR, int>) {
        return Int(op(l, r));
      } else if constexpr (is_same_v<TL, int> && is_same_v<TR, double>) {
        return Float(op(static_cast<double>(l), r));
      } else if constexpr (is_same_v<TL, double> && is_same_v<TR, int>) {
        return Float(op(l, static_cast<double>(r)));
      } else if constexpr (is_same_v<TL, double> && is_same_v<TR, double>) {
        return Float(op(l, r));
      } else {
        return nan();
      }
    };
    return var2::visit(visitor, l.val, r.val);
  }
};
}// namespace common


template<>
struct [[maybe_unused]] fmt::formatter<common::event_data> {
  // Presentation format: `n` - normal with constructor, `m` - monpoly database
  // format

  char presentation = 'n';

  constexpr auto parse [[maybe_unused]] (format_parse_context &ctx)
  -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'm'))
      presentation = *it++;
    if (it != end && *it != '}')
      throw format_error(
        R"(invalid format: ":m" for monpoly, ":n" for normal )");
    return it;
  }

  template<typename FormatContext>
  auto format
    [[maybe_unused]] (const common::event_data &tab, FormatContext &ctx)
    -> decltype(ctx.out()) {
    using boost::variant2::get_if;
    // TODO: fix this - probably a compiler bug
    if (const int *iptr = get_if<int>(&tab.val)) {
      return format_to(ctx.out(), /*(presentation == 'n') ? "Int({})" :*/ "{}",
                       *iptr);
    } else if (const double *dptr = get_if<double>(&tab.val)) {
      return format_to(ctx.out(),
                       /*(presentation == 'n') ? "Str(\"{}\")" :*/ "{:.5f}",
                       *dptr);
    } else {
      const std::string *sptr = get_if<std::string>(&tab.val);
      return format_to(ctx.out(),
                       /*presentation == 'n' ? "Str(\"{}\")" :*/ "\"{}\"",
                       *sptr);
    }
  }
};

template<>
struct [[maybe_unused]] fmt::formatter<common::event_data_opt> {
  // Presentation format: `n` - normal with constructor, `m` - monpoly database
  // format

  char presentation = 'n';

  constexpr auto parse [[maybe_unused]] (format_parse_context &ctx)
  -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (it != end && (*it == 'm'))
      presentation = *it++;
    if (it != end && *it != '}')
      throw format_error(
        R"(invalid format: ":m" for monpoly, ":n" for normal )");
    return it;
  }

  template<typename FormatContext>
  auto format
    [[maybe_unused]] (const common::event_data_opt &tab, FormatContext &ctx)
    -> decltype(ctx.out()) {
    if (tab.tag == common::event_data_opt::HOLDS_INT) {
      return format_to(ctx.out(), "{}", tab.i);
    } else if (tab.tag == common::event_data_opt::HOLDS_FLOAT) {
      return format_to(ctx.out(), "{:.5f}", tab.d);
    } else {
      return format_to(ctx.out(), "\"{}\"", *tab.s);
    }
  }
};
#endif
