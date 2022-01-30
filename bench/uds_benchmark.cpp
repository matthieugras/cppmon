#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <benchmark/benchmark.h>
#include <chrono>
#include <deserialization.h>
#include <formula.h>
#include <thread>
#include <util.h>

using namespace std::chrono_literals;

ABSL_FLAG(std::string, socket_path, "cppmon_uds", "path to the socket");
ABSL_FLAG(std::string, formula_path, "input_formula",
          "path to the formula file");

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("cppmon uds interface benchmark");
  absl::ParseCommandLine(argc, argv);

  fo::Formula formula(read_file(absl::GetFlag(FLAGS_formula_path)));
  ipc::serialization::deserializer deser(absl::GetFlag(FLAGS_socket_path),
                                         fo::Formula::get_known_preds());
  size_t num_dbs = 0;
  while (true) {
    // std::this_thread::sleep_for(10ms);
    benchmark::DoNotOptimize(deser);
    int64_t wm;
    ipc::serialization::ts_database db;
    auto ret = deser.read_database(db, wm);
    benchmark::DoNotOptimize(ret);
    if (ret == 0) {
      fmt::print("received {} dbs\n", num_dbs);
      break;
      deser.send_eof();
    } else if (ret == 1) {
      num_dbs += db.second.size();
    }
  }
}
