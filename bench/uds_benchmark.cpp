#include <benchmark/benchmark.h>
#include <deserialization.h>

int main() {
  ipc::serialization::deserializer deser("cppmon_uds");
  while (true) {
    benchmark::DoNotOptimize(deser);
    auto ret = deser.read_database();
    benchmark::DoNotOptimize(ret);
    if (!ret)
      break;
  }
}