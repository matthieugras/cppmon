#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <random>
#include <table.h>
#include <utility>
#include <vector>


using std::vector;

static std::mt19937_64 rand_gen;

static table<size_t> random_table(size_t n_cols, size_t n_rows,
                                  size_t max_value) {
  table<size_t> rand_tab(n_cols);
  auto value_dist = std::uniform_int_distribution<size_t>(0, max_value);
  for (size_t i = 0; i < n_rows; ++i) {
    vector<size_t> row;
    row.reserve(n_cols);
    for (size_t j = 0; j < n_cols; ++j) {
      row.push_back(value_dist(rand_gen));
    }
    rand_tab.add_row(std::move(row));
  }
  return rand_tab;
}

static vector<size_t> sample_idxs(size_t n, size_t n_samples) {
  vector<size_t> idx;
  idx.reserve(n_samples);
  auto idx_sampler = std::uniform_int_distribution<size_t>(0, n - 1);
  for (size_t i = 0; i < n_samples; ++i)
    idx.push_back(idx_sampler(rand_gen));
  std::sort(idx.begin(), idx.end());
  return idx;
}

static void BM_table_join(benchmark::State &state) {
  // TODO: debug crash
  size_t output_size_sum = 0;
  size_t num_iters = 0;
  size_t n_common_cols = state.range(0), n_cols = state.range(1),
         n_rows = state.range(2), value_range = state.range(3);

  for (auto _ : state) {
    state.PauseTiming();
    auto tbl1 = random_table(n_cols, n_rows, value_range),
         tbl2 = random_table(n_cols, n_rows, value_range);
    auto idx1 = sample_idxs(n_cols, n_common_cols),
         idx2 = sample_idxs(n_cols, n_common_cols);
    state.ResumeTiming();
    auto tbl3 = tbl1.natural_join(tbl2, idx1, idx2);
    output_size_sum += tbl3.tab_size();
    num_iters++;
  }
  double avg_size = ((double) output_size_sum) / ((double) num_iters);
  state.counters["OutputSize"] =
    benchmark::Counter(avg_size, benchmark::Counter::kAvgThreads);
}

BENCHMARK(BM_table_join)
  ->ArgNames({"n_common_cols", "n_cols", "n_rows", "value_range"})
  ->Args({1, 5, 5, 3})
  ->Args({1, 5, 300, 15000})
  ->Args({1, 300, 5, 3})
  ->Args({7, 300, 300, 3})
  ->Args({1, 10, 300, 3})
  ->Unit(benchmark::kMicrosecond)
  ->Repetitions(3)
  ->ReportAggregatesOnly();

BENCHMARK_MAIN();