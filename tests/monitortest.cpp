#include <absl/container/flat_hash_map.h>
#include <database.h>
#include <event_data.h>
#include <fmt/core.h>
#include <formula.h>
#include <gtest/gtest.h>
#include <hash_cache.h>
#include <monitor.h>
#include <util.h>
#include <vector>


using namespace fo;

TEST(MState, DoesCompile) {
  using ev = common::event_data;
  table<ev> tab(
    2, {{ev::Int(1), ev::String("rofl")}, {ev::Float(20.4), ev::Int(2)}});
  table<ev> tab1(tab);
}
TEST(MState, Bla) {
  using monitor::monitor_db_from_parser_db;
  // P (x) S [2,4] Q(x)
  using ed = common::event_data;
  auto formula =
    Formula::Since(Interval(2, 4), Formula::Pred("P", {Term::Var(0)}),
                   Formula::Pred("Q", {Term::Var(0)}));
  auto mon = monitor::monitor(formula);
  auto db1 = monitor_db_from_parser_db(
    {{"Q", {{ed::String("a")}, {ed::String("b")}, {ed::String("c")}}}});
  auto res1 = mon.step(db1, make_vector(1UL));
  fmt::print("RESULT IS: {}\n", res1);
  auto db2 =
    monitor_db_from_parser_db({{"P", {{ed::String("b")}, {ed::String("c")}}}});
  auto res2 = mon.step(db2, make_vector(2UL));
  fmt::print("RESULT IS: {}\n", res2);
  auto db3 =
    monitor_db_from_parser_db({{"P", {{ed::String("b")}, {ed::String("c")}}},
                               {"Q", {{ed::String("a")}, {ed::String("b")}}}});
  auto res3 = mon.step(db3, make_vector(3UL));
  fmt::print("RESULT IS: {}\n", res3);
  auto db4 = monitor_db_from_parser_db({{"P", {{ed::String("a")}}}});
  auto res4 = mon.step(db4, make_vector(7UL));
  fmt::print("RESULT IS: {}\n", res4);
}

TEST(MState, HashCache) {
  using common::event_data;
  using common::hash_cached;
}