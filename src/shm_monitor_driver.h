#ifndef CPPMON_SHM_MONITOR_DRIVER_H
#define CPPMON_SHM_MONITOR_DRIVER_H

#include <filesystem>
#include <monitor.h>
#include <monitor_driver.h>


class shm_monitor_driver : public monitor_driver {
public:
  shm_monitor_driver(const std::filesystem::path &formula_path,
                     const std::filesystem::path &sig_path,
                     const std::string &shm_id, size_t shm_size);
  void do_monitor() override;

private:

  monitor::monitor monitor_;
};


#endif// CPPMON_SHM_MONITOR_DRIVER_H
