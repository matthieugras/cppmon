#include <config.h>
#include <database.h>
#include <file_monitor_driver.h>
#include <fmt/core.h>
#include <monitor.h>
#include <string>
#include <traceparser.h>
#include <util.h>

#ifdef USE_JEMALLOC
#include <absl/flags/flag.h>
extern "C" {
ABSL_FLAG(bool, dump_heap, false, "create a heap dump before terminating");
#include <jemalloc/jemalloc.h>
}
#endif

file_monitor_driver::file_monitor_driver(
  const std::filesystem::path &formula_path,
  const std::filesystem::path &sig_path, const std::filesystem::path &log_path,
  std::optional<std::string> verdict_path)
    : printer_(std::move(verdict_path)) {
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

file_monitor_driver::~file_monitor_driver() noexcept = default;

void file_monitor_driver::do_monitor() {
  using parse::timestamped_database;
  std::string db_str;
  while (std::getline(log_, db_str)) {
    // fmt::print("asking to parse string: {}\n", db_str);
    auto [ts, db] = parser_.parse_database(db_str);
    auto mon_db = monitor::monitor_db_from_parser_db(std::move(db));
    auto sats = monitor_.step(mon_db, make_vector(ts));
    printer_.print_verdict(sats);
  }
#ifdef USE_JEMALLOC
  if (absl::GetFlag(FLAGS_dump_heap))
    mallctl("prof.dump", NULL, NULL, NULL, 0);
#endif
  auto sats_last = monitor_.last_step();
  printer_.print_verdict(sats_last);
}
