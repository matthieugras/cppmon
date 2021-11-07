#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <file_monitor_driver.h>
#include <memory>
#include <shm_monitor_driver.h>
#include <string>

ABSL_FLAG(bool, use_shm, false, "use the shared memory monitoring interface");
ABSL_FLAG(size_t, shm_mem_size, 50000000,
          "size of the shared memory region in bytes");
ABSL_FLAG(std::string, shm_id, "cppmon_shm",
          "id for the shared memory region (any string is acceptable)");
ABSL_FLAG(std::string, formula, "formula.fo", "path to formula file");
ABSL_FLAG(std::string, log, "log.fo", "path to log in monpoly format");
ABSL_FLAG(std::string, sig, "formula.sig", "path to signature");

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("MFOTL monitor written in C++");
  absl::ParseCommandLine(argc, argv);
  std::unique_ptr<monitor_driver> driver;

  if (absl::GetFlag(FLAGS_use_shm)) {
    driver.reset(new shm_monitor_driver(
      absl::GetFlag(FLAGS_formula), absl::GetFlag(FLAGS_sig),
      absl::GetFlag(FLAGS_shm_id), absl::GetFlag(FLAGS_shm_mem_size)));
  } else {
    driver.reset(new file_monitor_driver(absl::GetFlag(FLAGS_formula),
                                         absl::GetFlag(FLAGS_sig),
                                         absl::GetFlag(FLAGS_log)));
  }
  driver->do_monitor();
}