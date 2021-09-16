#include <formula.h>
#include <gtest/gtest.h>

TEST(FormulaCorrectness, FreeVariablesCorrectness) {
  using namespace fo;
  {
    Formula f(Pred{"A", {Term{Const{5}}}});
    fv_set fvs1 = f.fvs(), fvs2 = {};
    EXPECT_EQ(fvs1, fvs2);
  }
  {
    Formula f(Pred{"A", {Term{Var{0}}}});
    fv_set fvs1 = f.fvs(), fvs2 = {0};
    EXPECT_EQ(fvs1, fvs2);
  }
  {
    Formula f(Pred{"A", {Term{Var{0}}}});
    fv_set fvs1 = f.fvs(), fvs2 = {0};
    EXPECT_EQ(fvs1, fvs2);
  }
}