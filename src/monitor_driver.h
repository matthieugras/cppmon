#ifndef CPPMON_MONITOR_DRIVER_H
#define CPPMON_MONITOR_DRIVER_H
#include <monitor.h>
#include <stdexcept>

void print_satisfactions(monitor::satisfactions &sats);

class monitor_driver {
public:
  virtual void do_monitor() = 0;
  virtual ~monitor_driver() noexcept {};
};

#endif// CPPMON_MONITOR_DRIVER_H
