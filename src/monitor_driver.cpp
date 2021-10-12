#include <cassert>
#include <fmt/core.h>
#include <monitor.h>
#include <monitor_driver.h>
#include <string>
#include <traceparser.h>
#include <util.h>

static void print_satisfactions(monitor::satisfactions &sats) {
  for (auto &[ts, tp, tbl] : sats) {
    if (tbl.empty())
      continue;
    std::sort(tbl.begin(), tbl.end());
    fmt::print("@{}. (time point {}):", ts, tp);
    if (tbl.size() == 1 && tbl[0].empty())
      fmt::print(" true\n");
    else {
      for (const auto &row : tbl) {
        fmt::print(" ({})", fmt::join(row, ","));
      }
      fmt::print("\n");
    }
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

void monitor_driver::do_monitor() {
  using parse::timestamped_database;
  std::string db_str;
  while (std::getline(log_, db_str)) {
    auto [ts, db] = parser_.parse_database(db_str);
    auto sats = monitor_.step(db, ts);
    print_satisfactions(sats);
  }
}
