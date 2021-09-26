#ifndef CPPMON_MONITOR_DRIVER_H
#define CPPMON_MONITOR_DRIVER_H

#include <filesystem>
#include <monitor.h>
#include <traceparser.h>
#include <fstream>

class monitor_driver {
public:
  monitor_driver(const std::filesystem::path &formula_path,
                 const std::filesystem::path &sig_path,
                 const std::filesystem::path &log_path);
  void do_monitor();

private:

  parse::trace_parser parser_;
  std::ifstream log_;
  monitor::monitor monitor_;
};


#endif// CPPMON_MONITOR_DRIVER_H
