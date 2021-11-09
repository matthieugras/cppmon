#ifndef CPPMON_UDS_MONITOR_DRIVER_H
#define CPPMON_UDS_MONITOR_DRIVER_H

#include <deserialization.h>
#include <event_data.h>
#include <filesystem>
#include <formula.h>
#include <monitor.h>
#include <monitor_driver.h>
#include <traceparser.h>
#include <type_traits>
#include <util.h>


class uds_monitor_driver : public monitor_driver {
public:
  uds_monitor_driver(const std::filesystem::path &formula_path,
                     const std::filesystem::path &sig_path,
                     const std::string &socket_path);
  void do_monitor() override;

private:
  monitor::monitor monitor_;
  parse::signature sig_;
  ipc::serialization::deserializer deser_;
};


#endif// CPPMON_UDS_MONITOR_DRIVER_H
