#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <differential_test.h>
#include <string>

ABSL_FLAG(std::string, cppmon_path, "cppmon", "Path to the cppmon executable");
ABSL_FLAG(std::string, monpoly_path, "monpoly",
          "Path to the monpoly executable");
ABSL_FLAG(std::string, gen_fma_path, "gen_fma",
          "Path to the gen_fma executable");
ABSL_FLAG(std::string, trace_gen_path, "generator.sh",
          "Path to the trace generator executable");
ABSL_FLAG(std::string, replayer_path, "replayer.sh",
          "Path to the replayer executable");

int main(int argc, char *argv[]) {
  namespace fs = std::filesystem;
  absl::SetProgramUsageMessage("Test cppmon against monpoly/verimon");
  absl::ParseCommandLine(argc, argv);
  testing::differential_test test(
    absl::GetFlag(FLAGS_cppmon_path), absl::GetFlag(FLAGS_monpoly_path),
    absl::GetFlag(FLAGS_gen_fma_path), absl::GetFlag(FLAGS_trace_gen_path),
    absl::GetFlag(FLAGS_replayer_path));
  test.run_tests();
}