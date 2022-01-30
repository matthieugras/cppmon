#include "uds_monitor_driver.h"
#include "deserialization.h"
#include <cstdio>

uds_monitor_driver::uds_monitor_driver(
  const std::filesystem::path &formula_path,
  const std::filesystem::path &sig_path, const std::string &socket_path,
  std::optional<std::string> verdict_path)
    : printer_(std::move(verdict_path)), saved_wm_(-1) {
  auto formula = fo::Formula(read_file(formula_path));
  sig_ = parse::signature_parser::parse(read_file(sig_path));
  monitor_ = monitor::monitor(formula);
  deser_.emplace(socket_path, fo::Formula::get_known_preds());
}

void uds_monitor_driver::do_monitor() {
  using record_type = ipc::serialization::deserializer::record_type;
  while (true) {
    ipc::serialization::ts_database tdb;
    int64_t wm;
    // fmt::print(stderr, "waiting for new input\n");
    switch (deser_->read_database(tdb, wm)) {
      case record_type::EOF_RECORD: {
        // fmt::print(stderr, "read eof\n");
        if (saved_wm_ != -1) {
          deser_->send_latency_marker(saved_wm_);
          saved_wm_ = -1;
        }
        auto sats = monitor_.last_step();
        printer_.print_verdict(sats);
        // fmt::print(stderr, "sending eof\n");
        deser_->send_eof();
        // fmt::print(stderr, "now returning\n");
        return;
      }
      case record_type::LATENCY_RECORD: {
        // fmt::print(stderr, "read lat marker\n");
        if (saved_wm_ != -1)
          throw std::runtime_error("2 watermarks without data\n");
        saved_wm_ = wm;
        break;
      }
      case record_type::DB_RECORD: {
        // fmt::print(stderr, "read new db\n");
        if (saved_wm_ != -1) {
          deser_->send_latency_marker(saved_wm_);
          saved_wm_ = -1;
        }
        auto &[db, ts] = tdb;
        auto sats = monitor_.step(db, ts);
        printer_.print_verdict(sats);
        break;
      }
      default:
        throw std::runtime_error("cannot happen");
    }
  }
}
