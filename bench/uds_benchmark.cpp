#include <benchmark/benchmark.h>
#include <chrono>
#include <deserialization.h>
#include <thread>

using namespace std::chrono_literals;


int main() {
  ipc::serialization::deserializer deser("cppmon_uds");
  size_t num_dbs = 0;
  while (true) {
    //std::this_thread::sleep_for(10ms);
    benchmark::DoNotOptimize(deser);
    auto ret = deser.read_database();
    benchmark::DoNotOptimize(ret);
    if (!ret) {
      fmt::print("received {} dbs\n", num_dbs);
      break;
    } else {
      //fmt::print("received ts: {}, db: {}\n", ret->second, ret->first);
      num_dbs += ret->second.size();
    }
  }
}