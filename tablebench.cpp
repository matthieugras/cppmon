#include <algorithm>
#include <benchmark/benchmark.h>
#include <cstddef>
#include <fmt/format.h>
#include <random>
#include <table.h>
#include <utility>
#include <vector>

using std::vector;

static std::mt19937_64 rand_gen;

static table<size_t> random_table(vector<size_t> other_cols,
                                  size_t n_common_cols, size_t n_cols,
                                  size_t n_rows, size_t max_value) {
  assert(other_cols.size() >= n_common_cols);
  assert(n_common_cols < n_cols);
  vector<vector<size_t>> data;
  data.reserve(n_rows);
  vector<size_t> selected_cols;
  selected_cols.reserve(n_cols);
  auto col_name_dist = std::uniform_int_distribution<size_t>(0);
  std::sample(other_cols.begin(), other_cols.end(),
              std::back_inserter(selected_cols), n_common_cols, rand_gen);
  assert(selected_cols.size() == n_common_cols);
  for (size_t i = n_common_cols; i < n_cols; ++i) {
    selected_cols.push_back(col_name_dist(rand_gen));
  }
  std::random_shuffle(selected_cols.begin(), selected_cols.end());
  auto value_dist = std::uniform_int_distribution<size_t>(0, max_value);
  for (size_t i = 0; i < n_rows; ++i) {
    vector<size_t> row;
    row.reserve(n_cols);
    for (size_t j = 0; j < std::max(n_cols, n_common_cols); ++j) {
      row.push_back(value_dist(rand_gen));
    }
    data.push_back(std::move(row));
  }
  return table<size_t>(selected_cols, data);
}

static void BM_table_join(benchmark::State &state) {
  size_t output_size_sum = 0;
  size_t num_iters = 0;
  for (auto _ : state) {
    state.PauseTiming();
    auto tbl1 = random_table(vector<size_t>(), 0, state.range(1),
                             state.range(2), state.range(3)),
         tbl2 = random_table(tbl1.var_names(), state.range(0), state.range(1),
                             state.range(2), state.range(3));
    state.ResumeTiming();
    auto tbl3 = tbl1.natural_join(tbl2);
    output_size_sum += tbl3.tab_size();
    num_iters++;
  }
  double avg_size = ((double)output_size_sum) / ((double)num_iters);
  state.counters["OutputSize"] =
      benchmark::Counter(avg_size, benchmark::Counter::kAvgThreads);
}

BENCHMARK(BM_table_join)
    ->ArgNames({"n_common_cols", "n_cols", "n_rows", "value_range"})
    ->Args({1, 5, 5, 3})
    ->Args({1, 5, 300, 15000})
    ->Args({1, 300, 5, 3})
    ->Args({1, 10, 300, 3})
    ->Unit(benchmark::kMicrosecond)
    ->Repetitions(3)
    ->ReportAggregatesOnly();

BENCHMARK_MAIN();