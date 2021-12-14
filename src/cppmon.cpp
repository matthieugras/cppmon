#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <config.h>
#include <file_monitor_driver.h>
#include <memory>
#include <string>
#ifdef ENABLE_SOCK_INTF
#include <uds_monitor_driver.h>
ABSL_FLAG(bool, use_socket, false,
          "use the unix domain socket monitoring interface");
ABSL_FLAG(std::string, socket_path, "cppmon_uds",
          "path of the unix socket to create");
#endif
ABSL_FLAG(std::string, formula, "formula.fo", "path to formula file");
ABSL_FLAG(std::string, log, "log.fo", "path to log in monpoly format");
ABSL_FLAG(std::string, sig, "formula.sig", "path to signature");

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("MFOTL monitor written in C++");
  absl::ParseCommandLine(argc, argv);
  std::unique_ptr<monitor_driver> driver;

#ifdef ENABLE_SOCK_INTF
  if (absl::GetFlag(FLAGS_use_socket)) {
    driver.reset(new uds_monitor_driver(absl::GetFlag(FLAGS_formula),
                                        absl::GetFlag(FLAGS_sig),
                                        absl::GetFlag(FLAGS_socket_path)));
  } else {
    driver.reset(new file_monitor_driver(absl::GetFlag(FLAGS_formula),
                                         absl::GetFlag(FLAGS_sig),
                                         absl::GetFlag(FLAGS_log)));
  }
#else
  driver.reset(new file_monitor_driver(absl::GetFlag(FLAGS_formula),
                                       absl::GetFlag(FLAGS_sig),
                                       absl::GetFlag(FLAGS_log)));
#endif

  driver->do_monitor();
}
