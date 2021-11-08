#ifndef CPPMON_SHM_MONITOR_DRIVER_H
#define CPPMON_SHM_MONITOR_DRIVER_H

#include <boost/variant2/variant.hpp>
#include <event_data.h>
#include <filesystem>
#include <formula.h>
#include <monitor.h>
#include <monitor_driver.h>
#include <shm_shared_state.h>
#include <traceparser.h>
#include <type_traits>
#include <util.h>


class shm_monitor_driver : public monitor_driver {
public:
  shm_monitor_driver(const std::filesystem::path &formula_path,
                     const std::filesystem::path &sig_path, std::string shm_id,
                     size_t shm_size);
  ~shm_monitor_driver() noexcept override;
  void do_monitor() override;

private:
  void setup_shared_memory(size_t shm_size);
  std::string shm_id_;
  monitor::monitor monitor_;
  parse::signature sig_;
  ipc::boost_ipc::managed_shared_memory segment_;
  ipc::shared_state *ipc_state_;
};


#endif// CPPMON_SHM_MONITOR_DRIVER_H
