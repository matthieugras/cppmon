#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <monitor_driver.h>
#include <string>

ABSL_FLAG(std::string, formula, "formula.fo", "path to formula file");
ABSL_FLAG(std::string, log, "log.fo", "path to log in monpoly format");
ABSL_FLAG(std::string, sig, "formula.sig", "path to signature");

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("MFOTL monitor written in C++");
  absl::ParseCommandLine(argc, argv);
  monitor_driver driver(absl::GetFlag(FLAGS_formula), absl::GetFlag(FLAGS_sig),
                        absl::GetFlag(FLAGS_log));
  driver.do_monitor();
}