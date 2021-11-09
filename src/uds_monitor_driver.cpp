#include "uds_monitor_driver.h"

uds_monitor_driver::uds_monitor_driver(
  const std::filesystem::path &formula_path,
  const std::filesystem::path &sig_path, const std::string &socket_path)
    : deser_(socket_path) {
  auto formula = fo::Formula(read_file(formula_path));
  sig_ = parse::signature_parser::parse(read_file(sig_path));
  monitor_ = monitor::monitor(formula);
}

void uds_monitor_driver::do_monitor() {
  while (true) {
    auto opt_res = deser_.read_database();
    if (opt_res) {
      auto &[db, ts] = *opt_res;
      auto sats = monitor_.step(db, ts);
      print_satisfactions(sats);
    } else {
      auto sats = monitor_.last_step();
      print_satisfactions(sats);
      return;
    }
  }
}
