#include <monitor_driver.h>

void print_satisfactions(monitor::satisfactions &sats) {
  for (auto &[ts, tp, tbl] : sats) {
    if (tbl.empty())
      continue;
    std::sort(tbl.begin(), tbl.end());
    fmt::print("@{} (time point {}):", ts, tp);
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