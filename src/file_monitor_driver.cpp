#include <database.h>
#include <file_monitor_driver.h>
#include <monitor.h>
#include <string>
#include <traceparser.h>
#include <util.h>

file_monitor_driver::file_monitor_driver(
  const std::filesystem::path &formula_path,
  const std::filesystem::path &sig_path,
  const std::filesystem::path &log_path) {
  namespace fs = std::filesystem;
  using parse::signature;
  using parse::signature_parser;
  using parse::trace_parser;

  std::string formula_file_content = read_file(formula_path);
  fo::Formula formula(formula_file_content);
  monitor::monitor tmp_mon(formula);
  signature sig = signature_parser::parse(read_file(sig_path));

  trace_parser db_parser(std::move(sig));
  std::ifstream log_file(log_path);
  log_ = std::move(log_file);
  parser_ = std::move(db_parser);
  monitor_ = std::move(tmp_mon);
}

file_monitor_driver::~file_monitor_driver() noexcept {}

void file_monitor_driver::do_monitor() {
  using parse::timestamped_database;
  std::string db_str;
  while (std::getline(log_, db_str)) {
    auto [ts, db] = parser_.parse_database(db_str);
    auto sats = monitor_.step(monitor::monitor_db_from_parser_db(std::move(db)),
                              make_vector(ts));
    print_satisfactions(sats);
  }
  auto sats_last = monitor_.last_step();
  print_satisfactions(sats_last);
}
