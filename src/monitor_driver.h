#ifndef CPPMON_MONITOR_DRIVER_H
#define CPPMON_MONITOR_DRIVER_H
#include <fmt/format.h>
#include <fmt/os.h>
#include <monitor.h>
#include <optional>
#include <stdexcept>

class verdict_printer {
public:
  explicit verdict_printer(std::optional<std::string> file_name);
  void print_verdict(monitor::satisfactions &sats);

private:
  template<typename... Args>
  void print(fmt::format_string<Args...> fmt, Args &&...args) {
    if (ofile_)
      ofile_->print(fmt, std::forward<Args>(args)...);
    else
      fmt::print(fmt, std::forward<Args>(args)...);
  }
  std::optional<fmt::ostream> ofile_;
};

class monitor_driver {
public:
  virtual void do_monitor() = 0;
  virtual ~monitor_driver() noexcept {};
};

#endif// CPPMON_MONITOR_DRIVER_H
