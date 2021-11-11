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
    auto ret = deser.read_database();
    benchmark::DoNotOptimize(ret);
    if (!ret) {
      fmt::print("received {} dbs\n", num_dbs);
      break;
    } else {
      // fmt::print("received ts: {}, db: {}\n", ret->second, ret->first);
      num_dbs += ret->second.size();
    }
  }
}