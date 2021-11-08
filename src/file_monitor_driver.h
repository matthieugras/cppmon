#ifndef CPPMON_FILE_MONITOR_DRIVER_H
#define CPPMON_FILE_MONITOR_DRIVER_H

#include <filesystem>
#include <monitor.h>
#include <traceparser.h>
#include <fstream>
#include <monitor_driver.h>

class file_monitor_driver: public monitor_driver {
public:
  file_monitor_driver(const std::filesystem::path &formula_path,
                 const std::filesystem::path &sig_path,
                 const std::filesystem::path &log_path);
  ~file_monitor_driver() noexcept override;
  void do_monitor() override;

private:
  parse::trace_parser parser_;
  std::ifstream log_;
  monitor::monitor monitor_;
};


#endif// CPPMON_FILE_MONITOR_DRIVER_H
