#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <benchmark/benchmark.h>
#include <chrono>
#include <deserialization.h>
#include <thread>

using namespace std::chrono_literals;

ABSL_FLAG(std::string, socket_path, "cppmon_uds", "path to the socket");

int main(int argc, char *argv[]) {
  absl::SetProgramUsageMessage("cppmon uds interface benchmark");
  absl::ParseCommandLine(argc, argv);

  ipc::serialization::deserializer deser(absl::GetFlag(FLAGS_socket_path));
  size_t num_dbs = 0;
  while (true) {
    // std::this_thread::sleep_for(10ms);
    benchmark::DoNotOptimize(deser);
    int64_t wm; ipc::serialization::ts_database db;
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