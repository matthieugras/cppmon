#include <formula.h>
#include <gtest/gtest.h>
#include <mformula.h>
#include <fmt/core.h>

TEST(MFormula, DoesCompile) {
  using namespace fo;
  using namespace mfo;
  using ev = fo::event_t;
  using tbl_impl::table;

  table<ev> tab(
    {1, 2}, {{ev::Int(1), ev::String("rofl")}, {ev::Float(20.4), ev::Int(2)}});
  table<ev> tab1(tab);
  fmt::print("{}", tab == tab1);
}