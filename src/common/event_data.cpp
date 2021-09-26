#include <event_data.h>
#include <string_view>

namespace common {
using std::string;
using std::string_view;
using std::literals::string_view_literals::operator""sv;
event_data::event_data(val_type &&val) noexcept : val(std::move(val)) {}

bool event_data::operator==(const event_data &other) const {
  return val == other.val;
}

event_data event_data::Int(int i) { return event_data(i); }

event_data event_data::String(string s) { return event_data(std::move(s)); }

event_data event_data::Float(double d) { return event_data(d); }

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
