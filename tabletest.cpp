#include <fmt/format.h>
#include <gtest/gtest.h>
#include <table.h>

TEST(Table, Equality) {
  {
    table<int> t1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      t2({1, 2, 4}, {{5, 6, 7}, {8, 9, 10}});
    EXPECT_NE(t1, t2);
  }
  {
    table<int> t1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      t2({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
    EXPECT_EQ(t1, t2);
  }
  {
    table<int> t1({}, {{}, {}, {}}), t2({}, {{}});
    EXPECT_EQ(t1, t2);
  }
  {
    table<int> t1({0, 1}, {{1, 2}, {1, 2}}), t2({0, 1}, {{1, 2}});
    EXPECT_EQ(t1, t2);
  }
  {
    table<int> t1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      t2({3, 2, 1}, {{7, 6, 5}, {10, 9, 8}});
    EXPECT_EQ(t1, t2);
  }
}

TEST(Table, Inequality) {
  {
    table<int> t1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      t2({3, 2, 1}, {{7, 6, 5}, {9, 9, 8}});
    EXPECT_NE(t1, t2);
  }
  {
    table<int> t1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}, {11, 12, 13}}),
      t2({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
    EXPECT_NE(t1, t2);
  }
  {
    table<int> t1({}, {{}}), t2({}, {});
    EXPECT_NE(t1, t2);
  }
}

TEST(Table, Join) {
  {
    table<int> t1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      t2({4, 5, 6}, {{10, 20, 30}, {4, 2, 1}}),
      t3({1, 2, 3, 4, 5, 6}, {
                               {5, 6, 7, 10, 20, 30},
                               {5, 6, 7, 4, 2, 1},
                               {8, 9, 10, 10, 20, 30},
                               {8, 9, 10, 4, 2, 1},
                             });
    EXPECT_EQ(t1.natural_join(t2), t3);
  }
  {
    table<int> t1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      t2({2, 5, 6}, {{10, 20, 30}, {4, 2, 1}}), t3({1, 2, 3, 5, 6}, {});
    EXPECT_EQ(t1.natural_join(t2), t3);
  }
  {
    table<int> t1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      t2({2, 5, 6}, {{10, 20, 30}, {6, 2, 1}}),
      t3({1, 2, 3, 5, 6}, {{5, 6, 7, 2, 1}});
    EXPECT_EQ(t1.natural_join(t2), t3);
  }
  {
    table<int> t1({6, 2, 3}, {{5, 6, 7}, {5, 9, 10}}),
      t2({4, 5, 6}, {{10, 20, 5}, {11, 2, 22}}),
      t3({2, 3, 4, 5, 6}, {{6, 7, 10, 20, 5}, {9, 10, 10, 20, 5}});
    EXPECT_EQ(t1.natural_join(t2), t3);
  }
  {
    table<int> t1({1, 2, 6, 4, 5},
                  {{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 23, 13, 14, 24}}),
      t2({3, 2, 5}, {{16, 2, 5}, {19, 2, 5}, {22, 23, 24}, {25, 26, 27}}),
      t3({1, 2, 3, 4, 5, 6},
         {{1, 2, 16, 4, 5, 3}, {1, 2, 19, 4, 5, 3}, {11, 23, 22, 14, 24, 13}});
    EXPECT_EQ(t1.natural_join(t2), t3);
  }
  {
    table<int> t1({3, 1, 2}, {{5, 6, 7}, {8, 9, 10}, {11, 12, 13}}),
      t2({}, {{}});
    EXPECT_EQ(t1.natural_join(t2), t1);
  }
  {
    int bla = INT_MAX;
    table<int> t1({3, 1, 2}, {{5, 6, 7}, {8, 9, 10}, {11, 12, 13}}), t2({}, {}),
      t3({1, 2, 3}, {});
    EXPECT_EQ(t1.natural_join(t2), t3);
  }
}

TEST(Table, Union) {
  {
    table<int> tab1({}, {}), tab2({}, {}), tab3({}, {});
    EXPECT_EQ(tab1.t_union(tab2), tab3);
    tab1.t_union_in_place(tab2);
    EXPECT_EQ(tab1, tab3);
  }
  {
    table<int> tab1({1, 2}, {{1, 2}}), tab2({1, 2}, {{1, 2}, {3, 4}}),
      tab3({1, 2}, {{3, 4}, {1, 2}});
    EXPECT_EQ(tab1.t_union(tab2), tab3);
    tab1.t_union_in_place(tab2);
    EXPECT_EQ(tab1, tab3);
  }
  {
    table<int> tab1({2, 3, 1}, {{4, 5, 6}, {7, 8, 9}}),
      tab2({3, 2, 1}, {{7, 6, 5}, {3, 2, 7}}),
      tab3({1, 2, 3}, {{6, 4, 5}, {7, 2, 3}, {9, 7, 8}, {5, 6, 7}});
    EXPECT_EQ(tab1.t_union(tab2), tab3);
    tab1.t_union_in_place(tab2);
    EXPECT_EQ(tab1, tab3);
  }
  {
    table<int> tab1({2, 3, 1}, {{4, 5, 6}, {7, 8, 9}}),
      tab2({3, 2, 1}, {{7, 6, 5}, {3, 2, 7}}),
      tab3({1, 2, 3}, {{6, 4, 5}, {7, 3, 2}, {9, 7, 8}, {5, 6, 7}});
    EXPECT_NE(tab1.t_union(tab2), tab3);
    tab1.t_union_in_place(tab2);
    EXPECT_NE(tab1, tab3);
  }
  {
    table<int> tab1({5, 20, 11, 9},
                    {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}}),
      tab2({11, 9, 20, 5},
           {{13, 14, 15, 16}, {17, 18, 19, 20}, {21, 22, 23, 24}}),
      tab3({5, 20, 11, 9}, {{5, 6, 7, 8},
                            {1, 2, 3, 4},
                            {16, 15, 13, 14},
                            {20, 19, 17, 18},
                            {9, 10, 11, 12},
                            {24, 23, 21, 22}});
    EXPECT_EQ(tab1.t_union(tab2), tab3);
    tab1.t_union_in_place(tab2);
    EXPECT_EQ(tab1, tab3);
  }
}