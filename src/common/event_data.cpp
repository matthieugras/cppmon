#include <event_data.h>
#include <string_view>
#include <util.h>

namespace common {
using std::string;
using std::string_view;
using std::literals::string_view_literals::operator""sv;

event_data::event_data() : tag(HOLDS_INT) {}

void event_data::do_move(event_data &&other) noexcept {
  if (other.tag == HOLDS_INT) {
    i = other.i;
  } else if (other.tag == HOLDS_FLOAT) {
    d = other.d;
  } else {
    if (tag == HOLD_STRING)
      *s = std::move(*other.s);
    else {
      s = other.s;
      other.s = nullptr;
    }
  }
  tag = other.tag;
}
void event_data::do_copy(const event_data &other) {
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
  tag = other.tag;
}

event_data &event_data::operator=(event_data &&other) noexcept {
  do_move(std::move(other));
  return *this;
}

event_data &event_data::operator=(const event_data &other) {
  do_copy(other);
  return *this;
}

event_data::event_data(const event_data &other) : event_data() {
  do_copy(other);
}

event_data::event_data(event_data &&other) noexcept : event_data() {
  do_move(std::move(other));
}

bool event_data::operator>(const event_data &other) const {
  return !(*this <= other);
}

event_data::~event_data() {
  if (tag == HOLD_STRING) {
    delete s;
  }
}

event_data event_data::Int(int i) {
  event_data tmp;
  tmp.tag = HOLDS_INT;
  tmp.i = i;
  return tmp;
}
event_data event_data::Float(double d) {
  event_data tmp;
  tmp.tag = HOLDS_FLOAT;
  tmp.d = d;
  return tmp;
}
event_data event_data::String(const std::string &s) {
  event_data tmp;
  tmp.tag = HOLD_STRING;
  tmp.s = new std::string(s);
  return tmp;
}
event_data event_data::String(std::string &&s) {
  event_data tmp;
  tmp.tag = HOLD_STRING;
  tmp.s = new std::string(std::move(s));
  return tmp;
}

bool event_data::operator==(const event_data &other) const {
  if (tag != other.tag)
    return false;
  if (tag == HOLDS_INT) {
    return i == other.i;
  } else if (tag == HOLDS_FLOAT) {
    return d == other.d;
  } else {
    return *s == *other.s;
  }
}

bool event_data::operator<=(const event_data &other) const {

  if (tag == HOLDS_INT) {
    if (other.tag == HOLDS_INT)
      return i <= other.i;
    else if (other.tag == HOLDS_FLOAT)
      return static_cast<double>(i) <= other.d;
    else
      return false;
  } else if (tag == HOLDS_FLOAT) {
    if (other.tag == HOLDS_INT)
      return d <= static_cast<double>(other.i);
    else if (other.tag == HOLDS_FLOAT)
      return d <= other.d;
    else
      return false;
  } else {
    if (other.tag == HOLDS_INT || other.tag == HOLDS_FLOAT)
      return false;
    else
      return *s <= *other.s;
  }
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
  if (tag == HOLDS_INT && other.tag == HOLDS_INT)
    return Int(i % other.i);
  else
    return nan();
}

event_data event_data::operator-() const {
  if (tag == HOLDS_INT) {
    return Int(-i);
  } else if (tag == HOLDS_FLOAT) {
    return Float(-d);
  } else {
    return nan();
  }
}

event_data event_data::nan() {
  return Float(std::numeric_limits<double>::quiet_NaN());
}

double event_data::to_double() const {
  if (tag == HOLDS_INT)
    return static_cast<double>(i);
  else if (tag == HOLDS_FLOAT)
    return d;
  else
    return std::numeric_limits<double>::quiet_NaN();
}

event_data event_data::int_to_float() const {
  assert(tag == HOLDS_INT);
  return Float(i);
}

event_data event_data::float_to_int() const {
  assert(tag == HOLDS_FLOAT);
  return Int(static_cast<int>(d));
}

const int *event_data::get_if_int() const {
  if (tag == HOLDS_INT)
    return &i;
  else
    return nullptr;
}

event_data event_data::from_json(const json &json_formula) {
  string_view event_ty = json_formula.at(0).get<string_view>();
  if (event_ty == "EInt"sv) {
    return Int(json_formula.at(1).get<int>());
  } else if (event_ty == "EFloat"sv) {
    return Float(json_formula.at(1).get<double>());
  } else if (event_ty == "EString"sv) {
    return String(json_formula.at(1).get<string>());
  } else {
    throw std::runtime_error("invalid event_data json");
  }
}
}// namespace common
