#include "uds_monitor_driver.h"
#include "deserialization.h"
#include <cstdio>

uds_monitor_driver::uds_monitor_driver(
  const std::filesystem::path &formula_path,
  const std::filesystem::path &sig_path, const std::string &socket_path,
  std::optional<std::string> verdict_path)
    : printer_(std::move(verdict_path)) {
  auto formula = fo::Formula(read_file(formula_path));
  sig_ = parse::signature_parser::parse(read_file(sig_path));
  monitor_ = monitor::monitor(formula);
  deser_.emplace(socket_path, fo::Formula::get_known_preds());
}

void uds_monitor_driver::do_monitor() {
  while (true) {
    auto opt_db = deser_->read_database();
    if (opt_db) {
      auto &[db, ts] = *opt_db;
      auto sats = monitor_.step(db, ts);
      printer_.print_verdict(sats);
    } else {
      auto sats = monitor_.last_step();
      printer_.print_verdict(sats);
      deser_->send_eof();
      return;
    }
  }
}
