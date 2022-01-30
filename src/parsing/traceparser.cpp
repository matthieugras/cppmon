#include <traceparser.h>

#include <fmt/core.h>
#include <lexy/action/parse.hpp>
#include <lexy/input/string_input.hpp>
#include <lexy_ext/report_error.hpp>

namespace parse {

signature signature_parser::parse(std::string_view sig) {
  auto input = lexy::string_input<lexy::ascii_encoding>(sig);
  auto parsed_sig =
    lexy::parse<signature_parser::sig_parse>(input, lexy_ext::report_error);
  if (!parsed_sig)
    throw std::runtime_error("failed to read signature");
  assert(parsed_sig.has_value());
  return parsed_sig.value();
}

timestamped_database trace_parser::parse_database(std::string_view db) {
  auto st = parse_state{sig_, pred_map_};
  auto input = lexy::string_input<lexy::ascii_encoding>(db);
  auto parsed_db =
    lexy::parse<trace_parser::ts_db_parse>(input, st, lexy_ext::report_error);
  if (!parsed_db || (parsed_db.error_count() > 0))
    throw std::runtime_error("failed to parse timestamped database");
  assert(parsed_db.has_value());
  return parsed_db.value();
}
}// namespace parse
