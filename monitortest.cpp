#include <fmt/core.h>
#include <formula.h>
#include <gtest/gtest.h>
#include <monitor.h>

TEST(MFormula, DoesCompile) {
  using namespace fo;
  using namespace monitor;
  using ev = fo::event_data;

  table<ev> tab(
    {1, 2}, {{ev::Int(1), ev::String("rofl")}, {ev::Float(20.4), ev::Int(2)}});
  table<ev> tab1(tab);
}