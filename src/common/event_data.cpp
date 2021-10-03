#include <event_data.h>
#include <functional>
#include <string_view>
#include <util.h>

namespace common {
using std::string;
using std::string_view;
using std::literals::string_view_literals::operator""sv;
event_data::event_data(val_type &&val) noexcept : val(std::move(val)) {}

bool event_data::operator==(const event_data &other) const {
  return val == other.val;
}

bool event_data::operator<=(const event_data &other) const {
  using std::is_same_v;
  auto visitor = [](auto &&l, auto &&r) -> bool {
    using TL = std::decay_t<decltype(l)>;
    using TR = std::decay_t<decltype(r)>;

    if constexpr (is_same_v<TL, int>) {
      if constexpr (is_same_v<TR, int>)
        return l <= r;
      else if constexpr (is_same_v<TR, double>)
        return static_cast<double>(l) <= r;
      else
        return false;
    } else if constexpr (is_same_v<TL, double>) {
      if constexpr (is_same_v<TR, int>)
        return l <= static_cast<double>(r);
      else if constexpr (is_same_v<TR, double>)
        return l <= r;
      else
        return false;
    } else {
      if constexpr (any_type_equal_v<TR, int, double>)
        return false;
      else
        return l <= r;
    }
  };
  return var2::visit(visitor, val, other.val);
}

bool event_data::operator<(const event_data &other) const {
  return *this <= other && !(other <= *this);
}

event_data event_data::operator+(const event_data &other) const {
  auto op = [](auto l, auto r) { return l + r; };
  return apply_arith_bin_op(op, *this, other);
}

event_data event_data::operator-(const event_data &other) const {
  auto op = [](auto l, auto r) { return l - r; };
  return apply_arith_bin_op(op, *this, other);
}

event_data event_data::operator*(const event_data &other) const {
  auto op = [](auto l, auto r) { return l * r; };
  return apply_arith_bin_op(op, *this, other);
}

event_data event_data::operator/(const event_data &other) const {
  auto op = [](auto l, auto r) { return l / r; };
  return apply_arith_bin_op(op, *this, other);
}

event_data event_data::operator%(const event_data &other) const {
  const auto *const l_ptr = var2::get_if<int>(&this->val);
  const auto *const r_ptr = var2::get_if<int>(&other.val);
  if (l_ptr && r_ptr)
    return Int(*l_ptr % *r_ptr);
  else
    return nan();
}

event_data event_data::operator-() const {
  if (const auto *iptr = var2::get_if<int>(&val)) {
    return Int(-(*iptr));
  } else if (const auto *dptr = var2::get_if<double>(&val)) {
    return Float(-(*dptr));
  } else {
    return nan();
  }
}

event_data event_data::Int(int i) { return event_data(i); }

event_data event_data::String(string s) { return event_data(std::move(s)); }

event_data event_data::Float(double d) { return event_data(d); }

event_data event_data::nan() {
  return Float(std::numeric_limits<double>::quiet_NaN());
}

event_data event_data::int_to_float() const {
  return Float(var2::get<int>(val));
}

event_data event_data::float_to_int() const {
  return Int(static_cast<int>(var2::get<double>(val)));
}

const int *event_data::get_if_int() const {
  return var2::get_if<int>(&val);
}

event_data event_data::from_json(const json &json_formula) {
  string_view event_ty = json_formula.at(0).get<string_view>();
  if (event_ty == "EInt"sv) {
    return event_data::Int(json_formula.at(1).get<int>());
  } else if (event_ty == "EFloat"sv) {
    return event_data::Float(json_formula.at(1).get<double>());
  } else if (event_ty == "EString"sv) {
    return event_data::String(json_formula.at(1).get<string>());
  } else {
    throw std::runtime_error("invalid event_data json");
  }
}
}// namespace common
