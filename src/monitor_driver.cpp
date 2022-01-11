#include <monitor_driver.h>

verdict_printer::verdict_printer(std::optional<std::string> file_name) {
  if (file_name)
    ofile_.emplace(fmt::output_file(std::move(*file_name)));
}

void verdict_printer::print_verdict(monitor::satisfactions &sats) {
  for (auto &[ts, tp, tbl] : sats) {
    if (tbl.empty())
      continue;
    std::sort(tbl.begin(), tbl.end());
    print("@{} (time point {}):", ts, tp);
    if (tbl.size() == 1 && tbl[0].empty()) {
      print(" true\n");
    } else {
      for (const auto &row : tbl)
        print(" ({})", fmt::join(row, ","));
      print("\n");
    }
  }
}
