#include <formula.h>
#include <gtest/gtest.h>
#include <fmt/core.h>
#include <fmt/ranges.h>

TEST(FormulaCorrectness, FreeVariablesCorrectness) {
  using namespace fo;
  {
    Formula f = Formula::Pred("A", {Term{Const{5}}});
    fv_set fvs1 = f.fvs(), fvs2 = {};
    EXPECT_EQ(fvs1, fvs2);
  }
  {
    Formula f = Formula::Pred("A", {Term{Var{0}}});
    fv_set fvs1 = f.fvs(), fvs2 = {0};
    EXPECT_EQ(fvs1, fvs2);
  }
  {
    // This test fails
    Formula f = Formula::Exists(Formula::Pred("A", {Term{Var{0}}}));
    fv_set fvs1 = f.fvs(), fvs2 = {};
    EXPECT_EQ(fvs1, fvs2);
  }
}