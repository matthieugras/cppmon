#include <fmt/core.h>
#include <fmt/ranges.h>
#include <formula.h>
#include <gtest/gtest.h>

TEST(Formula, FormulaFromJson) {
  using namespace fo;
  std::string_view f_json_rv_11_b_neg =
    R"(["And",["Pred","publish",[["Var",["Nat",0]],["Var",["Nat",1]]]],
          ["Neg",
            ["Since",
              ["Eq",
                ["Const",["EInt",0]],
                ["Const",["EInt",0]]],
              [["Nat",0],["Enat",["Nat",604800]]],
              ["Exists",
                ["And",
                  ["Since",
                      ["Neg",
                        ["Pred","mgr_F",[["Var",["Nat",0]], ["Var",["Nat",1]]]]],
                      [["Nat",0],["Infinity_enat"]],
                      ["Pred","mgr_S",[["Var",["Nat",0]],["Var",["Nat",1]]]]],
                  ["Pred","approve",[
                    ["Var",["Nat",0]],
                    ["Var",["Nat",2]]]]]]]]])";
  std::string_view ex_mfotl =
    R"(["Until",
          ["Pred","P",[["Var",["Nat",0]]]],
          [["Nat",10],["Enat",["Nat",20]]],
          ["Pred","Q",[["Var",["Nat",0]],["Var",["Nat",1]]]]])";
  std::string_view exp2_p4 =
    R"(["And",
          ["Exists",
            ["And",
              ["And",
                ["Pred","trans",[["Var",["Nat",1]],["Var",["Nat",2]],["Var",["Nat",3]]]],
                ["Since",
                  ["Eq",
                    ["Const",["EInt",0]],
                    ["Const",["EInt",0]]],
                [["Nat",0],["Enat",["Nat",30]]],
                ["And",
                  ["Exists",
                    ["Pred","trans",[["Var",["Nat",2]],["Var",["Nat",1]],["Var",["Nat",0]]]]],
                  ["Until",
                    ["Eq",
                      ["Const",["EInt",0]],
                      ["Const",["EInt",0]]],
                    [["Nat",0],["Enat",["Nat",5]]],
                    ["Pred","report",[["Var",["Nat",0]]]]]]]],
              ["Neg",["Eq",["Var",["Nat",2]],["Var",["Nat",0]]]]]],
          ["Neg",
            ["Until",
              ["Eq",
                ["Const",["EInt",0]],
                ["Const",["EInt",0]]],
              [["Nat",0],["Enat",["Nat",2]]],
              ["Pred","report",[["Var",["Nat",1]]]]]]])";
  auto f_json_rv_11_b_neg_parsed = Formula(f_json_rv_11_b_neg);
  {
    auto f = Formula::And(
      Formula::Pred("publish", {Term::Var(0), Term::Var(1)}),
      Formula::Neg(Formula::Since(
        Interval(0, 604800),
        Formula::Eq(Term::Const(event_t::Int(0)), Term::Const(event_t::Int(0))),
        Formula::Exists(Formula::And(
          Formula::Since(
            Interval(0, 0, false),
            Formula::Neg(Formula::Pred("mgr_F", {Term::Var(0), Term::Var(1)})),
            Formula::Pred("mgr_S", {Term::Var(0), Term::Var(1)})),
          Formula::Pred("approve", {Term::Var(0), Term::Var(2)}))))));
    EXPECT_EQ(f, f_json_rv_11_b_neg_parsed);
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::And(
      Formula::Pred("publish", {Term::Var(1), Term::Var(1)}),
      Formula::Neg(Formula::Since(
        Interval(0, 604800),
        Formula::Eq(Term::Const(event_t::Int(0)), Term::Const(event_t::Int(0))),
        Formula::Exists(Formula::And(
          Formula::Since(
            Interval(0, 0, false),
            Formula::Neg(Formula::Pred("mgr_F", {Term::Var(0), Term::Var(1)})),
            Formula::Pred("mgr_S", {Term::Var(0), Term::Var(1)})),
          Formula::Pred("approve", {Term::Var(0), Term::Var(2)}))))));
    EXPECT_NE(f, f_json_rv_11_b_neg_parsed);
  }
  {
    auto f = Formula::And(
      Formula::Pred("publish", {Term::Var(0), Term::Var(1)}),
      Formula::Neg(Formula::Since(
        Interval(0, 6048005),
        Formula::Eq(Term::Const(event_t::Int(0)), Term::Const(event_t::Int(0))),
        Formula::Exists(Formula::And(
          Formula::Since(
            Interval(0, 0, false),
            Formula::Neg(Formula::Pred("mgr_F", {Term::Var(0), Term::Var(1)})),
            Formula::Pred("mgr_S", {Term::Var(0), Term::Var(1)})),
          Formula::Pred("approve", {Term::Var(0), Term::Var(2)}))))));
    EXPECT_NE(f, f_json_rv_11_b_neg_parsed);
  }
  {
    auto f = Formula::And(
      Formula::Pred("publish", {Term::Var(0), Term::Var(1)}),
      Formula::Neg(Formula::Since(
        Interval(0, 604800),
        Formula::Eq(Term::Const(event_t::Int(0)),
                    Term::Const(event_t::Float(2.5))),
        Formula::Exists(Formula::And(
          Formula::Since(
            Interval(0, 0, false),
            Formula::Neg(Formula::Pred("mgr_F", {Term::Var(0), Term::Var(1)})),
            Formula::Pred("mgr_S", {Term::Var(0), Term::Var(1)})),
          Formula::Pred("approve", {Term::Var(0), Term::Var(2)}))))));
    EXPECT_NE(f, f_json_rv_11_b_neg_parsed);
  }
  {
    auto f = Formula::And(
      Formula::Pred("publish", {Term::Var(0), Term::Var(1)}),
      Formula::Neg(Formula::Since(
        Interval(0, 604800),
        Formula::Eq(Term::Const(event_t::Int(0)), Term::Const(event_t::Int(0))),
        Formula::Exists(Formula::And(
          Formula::Since(
            Interval(0, 0, false),
            Formula::Neg(Formula::Pred("mgr_F", {Term::Var(0), Term::Var(1)})),
            Formula::Pred("mgr_Sk", {Term::Var(0), Term::Var(1)})),
          Formula::Pred("approve", {Term::Var(0), Term::Var(2)}))))));
    EXPECT_NE(f, f_json_rv_11_b_neg_parsed);
  }
  {
    auto f =
      Formula::Until(Interval(10, 20), Formula::Pred("P", {Term::Var(0)}),
                     Formula::Pred("Q", {Term::Var(0), Term::Var(1)}));
    EXPECT_EQ(f, Formula(ex_mfotl));
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::And(
      Formula::Exists(Formula::And(
        Formula::And(
          Formula::Pred("trans", {Term::Var(1), Term::Var(2), Term::Var(3)}),
          Formula::Since(
            Interval(0, 30),
            Formula::Eq(Term::Const(event_t::Int(0)),
                        Term::Const(event_t::Int(0))),
            Formula::And(
              Formula::Exists(Formula::Pred(
                "trans", {Term::Var(2), Term::Var(1), Term::Var(0)})),
              Formula::Until(Interval(0, 5),
                             Formula::Eq(Term::Const(event_t::Int(0)),
                                         Term::Const(event_t::Int(0))),
                             Formula::Pred("report", {Term::Var(0)}))))),
        Formula::Neg(Formula::Eq(Term::Var(2), Term::Var(0))))),
      Formula::Neg(Formula::Until(
        Interval(0, 2),
        Formula::Eq(Term::Const(event_t::Int(0)), Term::Const(event_t::Int(0))),
        Formula::Pred("report", {Term::Var(1)}))));
    EXPECT_TRUE(f.is_safe_formula());
    EXPECT_EQ(f, Formula(exp2_p4));
  }
}


TEST(Formula, FreeVariables) {
  using namespace fo;
  {
    auto f = Formula::Pred("A", {});
    fv_set fvs1 = f.fvs(), fvs2 = {};
    EXPECT_EQ(f.degree(), 0UL);
    EXPECT_EQ(fvs1, fvs2);
  }
  {
    auto f = Formula::Pred("A", {Term::Var(0)});
    fv_set fvs1 = f.fvs(), fvs2 = {0};
    EXPECT_EQ(f.degree(), 1UL);
    EXPECT_EQ(fvs1, fvs2);
  }
  {
    auto f = Formula::Exists(Formula::Pred("A", {Term::Var(0)}));
    fv_set fvs1 = f.fvs(), fvs2 = {};
    EXPECT_EQ(fvs1, fvs2);
    EXPECT_EQ(f.degree(), 0UL);
  }
  {
    auto f = Formula::Exists(Formula::Pred("A", {Term::Var(0), Term::Var(1)}));
    fv_set fvs1 = f.fvs(), fvs2 = {0};
    EXPECT_EQ(fvs1, fvs2);
    EXPECT_EQ(f.degree(), 1UL);
  }
  {
    auto f = Formula::Exists(Formula::And(
      Formula::Pred("P", {Term::Var(0), Term::Var(0)}),
      Formula::Exists(Formula::Pred("Q", {Term::Var(1), Term::Var(0)}))));
    fv_set fvs1 = f.fvs(), fvs2 = {};
    EXPECT_EQ(fvs1, fvs2);
    EXPECT_EQ(f.degree(), 0UL);
  }
  {
    auto f = Formula(
      R"(["Exists", ["And",["Exists",["And",["Pred","P",[["Var",["Nat",1]],["Var",["Nat",1]],
                    ["Var",["Nat",1]]]],["Pred","P",[["Var",["Nat",2]],["Var",["Nat",3]],["Var",["Nat",0]]]]]]
                    ,["Pred","P",[["Var",["Nat",3]],["Var",["Nat",2]],["Var",["Nat",1]]]]]])");
    fv_set fvs1 = f.fvs(), fvs2 = {0, 1, 2};
    EXPECT_EQ(fvs1, fvs2);
    EXPECT_EQ(f.degree(), 3UL);
  }
  {
    auto f = Formula(
      R"(["Exists",["And",["Exists",["And",["Pred","P",[["Var",["Nat",1]],["Var",["Nat",1]],
                    ["Var",["Nat",1]]]],["Pred","P",[["Var",["Nat",2]],["Var",["Nat",3]],["Var",["Nat",0]]]]]],
                    ["Pred","P",[["Var",["Nat",3]],["Var",["Nat",0]],["Var",["Nat",1]]]]]])");
    fv_set fvs1 = f.fvs(), fvs2 = {0, 1, 2};
    EXPECT_EQ(fvs1, fvs2);
    EXPECT_EQ(f.degree(), 3UL);
  }
  {
    auto f = Formula(
      R"(["Exists",["And",["Exists",["And",["Pred","P",[["Var",["Nat",1]],["Var",["Nat",1]],
                    ["Var",["Nat",1]]]],["Pred","P",[["Var",["Nat",2]],["Var",["Nat",3]],["Var",["Nat",0]]]]]],
                    ["Pred","P",[["Var",["Nat",0]],["Var",["Nat",0]],["Var",["Nat",1]]]]]])");
    fv_set fvs1 = f.fvs(), fvs2 = {0, 1};
    EXPECT_EQ(fvs1, fvs2);
    EXPECT_EQ(f.degree(), 2UL);
  }
}

TEST(Formula, IsSafeFormula) {
  using namespace fo;
  {
    auto f = Formula::Eq(Term::Var(0), Term::Var(1));
    EXPECT_FALSE(f.is_safe_formula());
  }
  {
    auto f = Formula::Eq(Term::Const(event_t::Int(1)), Term::Var(0));
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::Neg(Formula::Pred("A", {Term::Var(0)}));
    EXPECT_FALSE(f.is_safe_formula());
  }
  {
    auto f = Formula::Neg(Formula::Pred("A", {Term::Const(event_t::Int(1))}));
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::Prev(Interval(0, 0, false),
                           Formula::Pred("A", {Term::I2f(Term::Var(0))}));
    EXPECT_FALSE(f.is_safe_formula());
  }
  {
    auto f =
      Formula::Prev(Interval(0, 0, false), Formula::Pred("A", {Term::Var(0)}));
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::Exists(
      Formula::Next(Interval(0, 0, true),
                    Formula::Neg(Formula::Eq(Term::Var(0), Term::Var(1)))));
    EXPECT_FALSE(f.is_safe_formula());
  }
  {
    auto f = Formula::Exists(
      Formula::Next(Interval(0, 0, true),
                    Formula::Neg(Formula::Eq(Term::Var(1), Term::Var(1)))));
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::Next(
      Interval(0, 0, true),
      Formula::Neg(Formula::Exists(Formula::Pred("a", {Term::Var(0)}))));
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::Next(
      Interval(0, 0, true),
      Formula::Neg(Formula::Exists(Formula::Pred("a", {Term::Var(1)}))));
    EXPECT_FALSE(f.is_safe_formula());
  }
  {
    auto f = Formula::Exists(
      Formula::Or(Formula::Pred("A", {Term::Var(0), Term::Var(1)}),
                  Formula::Pred("B", {Term::Var(1), Term::Var(0)})));
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::Or(Formula::Exists(Formula::Pred(
                           "A", {Term::Var(0), Term::Var(1), Term::Var(2)})),
                         Formula::Pred("B", {Term::Var(0), Term::Var(1)}));
    EXPECT_TRUE(f.is_safe_formula());
  }
  {
    auto f = Formula::Or(
      Formula::Exists(Formula::Pred("A", {Term::Var(2), Term::Var(3)})),
      Formula::Pred("B", {Term::Var(0), Term::Var(1)}));
    EXPECT_FALSE(f.is_safe_formula());
  }
  // TODO: Add tests for SINCE, UNTIL and AND
}