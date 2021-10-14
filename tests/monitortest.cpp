#include <event_data.h>
#include <fmt/core.h>
#include <formula.h>
#include <gtest/gtest.h>
#include <monitor.h>
#include <vector>
#include <hash_cache.h>
#include <absl/container/flat_hash_map.h>


using namespace fo;

TEST(MState, DoesCompile) {
  using ev = common::event_data;
  table<ev> tab(
    2, {{ev::Int(1), ev::String("rofl")}, {ev::Float(20.4), ev::Int(2)}});
  table<ev> tab1(tab);
}
TEST(MState, Bla) {
  // P (x) S [2,4] Q(x)
  using ed = common::event_data;
  auto formula =
    Formula::Since(Interval(2, 4), Formula::Pred("P", {Term::Var(0)}),
                   Formula::Pred("Q", {Term::Var(0)}));
  auto mon = monitor::monitor(formula);
  auto res1 = mon.step(
    {{"Q", {{ed::String("a")}, {ed::String("b")}, {ed::String("c")}}}}, 1);
  fmt::print("RESULT IS: {}\n", res1);
  auto res2 = mon.step({{"P", {{ed::String("b")}, {ed::String("c")}}}}, 2);
  fmt::print("RESULT IS: {}\n", res2);
  auto res3 = mon.step({{"P", {{ed::String("b")}, {ed::String("c")}}},
                        {"Q", {{ed::String("a")}, {ed::String("b")}}}},
                       3);
  fmt::print("RESULT IS: {}\n", res3);
  auto res4 = mon.step({{"P", {{ed::String("a")}}}}, 7);
  fmt::print("RESULT IS: {}\n", res4);
}

TEST(MState, HashCache) {
  using common::event_data;
  using common::hash_cached;
}