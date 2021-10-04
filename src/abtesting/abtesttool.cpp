#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>


int main(int argc, char *argv[]) {
  namespace fs = std::filesystem;
  absl::SetProgramUsageMessage(
    "Test cppmon against monpoly/verimon");
  absl::ParseCommandLine(argc, argv);

}