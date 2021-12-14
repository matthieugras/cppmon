#include "uds_monitor_driver.h"

uds_monitor_driver::uds_monitor_driver(
  const std::filesystem::path &formula_path,
  const std::filesystem::path &sig_path, const std::string &socket_path)
    : deser_(socket_path), saved_wm_(-1) {
  auto formula = fo::Formula(read_file(formula_path));
  sig_ = parse::signature_parser::parse(read_file(sig_path));
  monitor_ = monitor::monitor(formula);
}

void uds_monitor_driver::do_monitor() {
  while (true) {
    ipc::serialization::ts_database tdb; int64_t wm;
    int ret_code = deser_.read_database(tdb, wm);
    if (ret_code == 0) {
      if (saved_wm_ != -1) {
        // fmt::print("sending back watermark {}\n", saved_wm_);
        deser_.send_latency_marker(saved_wm_);
        saved_wm_ = -1;
      }
      auto sats = monitor_.last_step();
      print_satisfactions(sats);
      deser_.send_eof();
      return;
    } else if (ret_code == 1) {
      if (saved_wm_ != -1) {
        // fmt::print("sending back watermark {}\n", saved_wm_);
        deser_.send_latency_marker(saved_wm_);
        saved_wm_ = -1;
      }
      auto &[db, ts] = tdb;
      auto sats = monitor_.step(db, ts);
      print_satisfactions(sats);
    } else {
      // fmt::print("received new watermark {}\n", saved_wm_);
      if (saved_wm_ != -1)
        throw std::runtime_error("2 watermarks without data\n");
      saved_wm_ = wm;
    }
  }
}
