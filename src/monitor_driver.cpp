#include <cassert>
#include <fmt/core.h>
#include <monitor.h>
#include <monitor_driver.h>
#include <string>
#include <traceparser.h>

static void print_satisfactions(const monitor::satisfactions &sats, size_t ts) {
  for (const auto &[tp, tbl] : sats) {
    fmt::print("TS: {}, TP: {}, {}\n", ts, tp, tbl);
  }
}

monitor_driver::monitor_driver(const std::filesystem::path &formula_path,
                               const std::filesystem::path &sig_path,
                               const std::filesystem::path &log_path) {
  namespace fs = std::filesystem;
  using parse::signature;
  using parse::signature_parser;
  using parse::trace_parser;

  assert(fs::exists(formula_path) && fs::exists(sig_path) &&
         fs::exists(log_path) && fs::is_regular_file(formula_path) &&
         fs::is_regular_file(sig_path) && fs::is_regular_file(log_path));

  monitor::monitor tmp_mon;
  signature sig;

  {
    std::ifstream sig_file(sig_path);
    std::stringstream buf;
    buf << sig_file.rdbuf();
    sig = signature_parser::parse(buf.view());
  }

  trace_parser db_parser(std::move(sig));
  std::ifstream log_file(log_path);
  log_ = std::move(log_file);
  parser_ = std::move(db_parser);
}

void monitor_driver::do_monitor() {
  using parse::timestamped_database;
  std::string db_str;
  while (std::getline(log_, db_str)) {
    auto [ts, db] = parser_.parse_database(db_str);
    auto sats = monitor_.step(db, ts);
    print_satisfactions(sats, ts);
  }
}
