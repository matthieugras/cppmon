#ifndef CPPMON_FILE_MONITOR_DRIVER_H
#define CPPMON_FILE_MONITOR_DRIVER_H

#include <filesystem>
#include <fstream>
#include <monitor.h>
#include <monitor_driver.h>
#include <traceparser.h>

class file_monitor_driver : public monitor_driver {
public:
  file_monitor_driver(const std::filesystem::path &formula_path,
                      const std::filesystem::path &sig_path,
                      const std::filesystem::path &log_path,
                      std::optional<std::string> verdict_path);
  ~file_monitor_driver() noexcept override;
  void do_monitor() override;

private:
  parse::trace_parser parser_;
  std::ifstream log_;
  monitor::monitor monitor_;
  verdict_printer printer_;
};


#endif// CPPMON_FILE_MONITOR_DRIVER_H
