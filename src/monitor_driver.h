#ifndef CPPMON_MONITOR_DRIVER_H
#define CPPMON_MONITOR_DRIVER_H
#include <stdexcept>

class monitor_driver {
public
  virtual void do_monitor() = 0;
};

#endif// CPPMON_MONITOR_DRIVER_H
