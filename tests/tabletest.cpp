#include <fmt/format.h>
#include <gtest/gtest.h>
#include <table.h>

TEST(Table, Equality) {
  {
    table_layout l1 = {1, 2, 3}, l2 = {1, 2, 3};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}}), t2(3, {{5, 6, 7}, {8, 9, 10}});
    EXPECT_TRUE(t1.equal_to(t2, permutation));
  }
  {
    table_layout l1 = {}, l2 = {};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(0, {{}, {}, {}}), t2(0, {{}});
    EXPECT_TRUE(t1.equal_to(t2, permutation));
  }
  {
    table_layout l1 = {0, 1}, l2 = {0, 1};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(2, {{1, 2}, {1, 2}}), t2(2, {{1, 2}});
    EXPECT_TRUE(t1.equal_to(t2, permutation));
  }
  {
    table_layout l1 = {1, 2, 3}, l2 = {3, 2, 1};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}}), t2(3, {{7, 6, 5}, {10, 9, 8}});
    EXPECT_TRUE(t1.equal_to(t2, permutation));
  }
  {
    table_layout l1 = {50, 10, 2, 6}, l2 = {10, 6, 50, 2};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(4, {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}}),
      t2(4, {{2, 4, 1, 3}, {6, 8, 5, 7}, {10, 12, 9, 11}});
    EXPECT_TRUE(t1.equal_to(t2, permutation));
  }
  {
    table_layout l1 = {50, 10, 2, 6}, l2 = {10, 6, 50, 2};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(4, {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}}),
      t2(4, {{2, 4, 1, 3}, {6, 8, 5, 7}, {2, 4, 1, 3}, {10, 12, 9, 11}});
    EXPECT_TRUE(t1.equal_to(t2, permutation));
  }
}


TEST(Table, Inequality) {
  {
    table_layout l1 = {1, 2, 3}, l2 = {1, 2, 3};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}}), t2(3, {{5, 6, 7}, {7, 9, 10}});
    EXPECT_FALSE(t1.equal_to(t2, permutation));
  }
  {
    table_layout l1 = {1, 2, 3}, l2 = {3, 2, 1};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}}), t2(3, {{7, 6, 5}, {9, 9, 8}});
    EXPECT_FALSE(t1.equal_to(t2, permutation));
  }
  {
    table_layout l1 = {1, 2, 3}, l2 = {1, 2, 3};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}, {11, 12, 13}}),
      t2(3, {{5, 6, 7}, {8, 9, 10}});
    EXPECT_FALSE(t1.equal_to(t2, permutation));
  }
  {
    table_layout l1 = {}, l2 = {};
    auto permutation = find_permutation(l1, l2);
    table<int> t1(0, {{}}), t2(0, {});
    EXPECT_FALSE(t1.equal_to(t2, permutation));
  }
}


TEST(Table, Join) {
  {
    table_layout l1 = {1, 2, 3}, l2 = {4, 5, 6}, l3 = {1, 2, 3, 4, 5, 6};
    auto [pred_lay, idx1, idx2] = get_join_layout(l1, l2);
    EXPECT_EQ(pred_lay, l3);
    auto permutation = id_permutation(6);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}}), t2(3, {{10, 20, 30}, {4, 2, 1}}),
      t3(6, {
              {5, 6, 7, 10, 20, 30},
              {5, 6, 7, 4, 2, 1},
              {8, 9, 10, 10, 20, 30},
              {8, 9, 10, 4, 2, 1},
            });
    EXPECT_TRUE(t1.natural_join(t2, idx1, idx2).equal_to(t3, permutation));
  }
  {
    table_layout l1 = {1, 2, 3}, l2 = {2, 5, 6}, l3 = {1, 2, 3, 5, 6};
    auto [pred_lay, idx1, idx2] = get_join_layout(l1, l2);
    auto permutation = find_permutation(pred_lay, l3);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}}), t2(3, {{10, 20, 30}, {4, 2, 1}}),
      t3(5, {});
    EXPECT_TRUE(t1.natural_join(t2, idx1, idx2).equal_to(t3, permutation));
  }
  {
    table_layout l1 = {1, 2, 3}, l2 = {2, 5, 6}, l3 = {1, 2, 3, 5, 6};
    auto [pred_lay, idx1, idx2] = get_join_layout(l1, l2);
    auto permutation = find_permutation(pred_lay, l3);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}}), t2(3, {{10, 20, 30}, {6, 2, 1}}),
      t3(5, {{5, 6, 7, 2, 1}});
    EXPECT_TRUE(t1.natural_join(t2, idx1, idx2).equal_to(t3, permutation));
  }
  {
    table_layout l1 = {6, 2, 3}, l2 = {4, 5, 6}, l3 = {2, 3, 4, 5, 6};
    auto [pred_lay, idx1, idx2] = get_join_layout(l1, l2);
    auto permutation = find_permutation(pred_lay, l3);
    table<int> t1(3, {{5, 6, 7}, {5, 9, 10}}),
      t2(3, {{10, 20, 5}, {11, 2, 22}}),
      t3(5, {{6, 7, 10, 20, 5}, {9, 10, 10, 20, 5}});
    EXPECT_TRUE(t1.natural_join(t2, idx1, idx2).equal_to(t3, permutation));
  }
  {
    table_layout l1 = {1, 2, 6, 4, 5}, l2 = {3, 2, 5}, l3 = {1, 2, 3, 4, 5, 6};
    auto [pred_lay, idx1, idx2] = get_join_layout(l1, l2);
    auto permutation = find_permutation(pred_lay, l3);
    table<int> t1(5, {{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 23, 13, 14, 24}}),
      t2(3, {{16, 2, 5}, {19, 2, 5}, {22, 23, 24}, {25, 26, 27}}),
      t3(6,
         {{1, 2, 16, 4, 5, 3}, {1, 2, 19, 4, 5, 3}, {11, 23, 22, 14, 24, 13}});
    EXPECT_TRUE(t1.natural_join(t2, idx1, idx2).equal_to(t3, permutation));
  }
  /*
   * Unit tables always have 0 cols and 1 row, empty tables can have any number
   * of columns
   */
  {
    table_layout l1 = {3, 1, 2}, l2 = {};
    auto [pred_lay, idx1, idx2] = get_join_layout(l1, l2);
    auto permutation = find_permutation(pred_lay, l1);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}, {11, 12, 13}}), t2(0, {{}});
    EXPECT_TRUE(t1.natural_join(t2, idx1, idx2).equal_to(t1, permutation));
  }
  {
    table_layout l1 = {3, 1, 2}, l2 = {}, l3 = {1, 2, 3};
    auto [pred_lay, idx1, idx2] = get_join_layout(l1, l2);
    auto permutation = find_permutation(pred_lay, l3);
    table<int> t1(3, {{5, 6, 7}, {8, 9, 10}, {11, 12, 13}}), t2(0, {}),
      t3(3, {});
    EXPECT_TRUE(t1.natural_join(t2, idx1, idx2).equal_to(t3, permutation));
  }
}

TEST(Table, Union) {
  {
    table_layout l1 = {}, l2 = {};
    auto permutation = id_permutation(0);
    auto tab1 = table<int>::empty_table(), tab2 = table<int>::empty_table(),
         tab3 = table<int>::empty_table();
    EXPECT_TRUE(tab1.t_union(tab2, permutation).equal_to(tab3, permutation));
    tab1.t_union_in_place(tab2, permutation);
    EXPECT_TRUE(tab1.equal_to(tab3, permutation));
  }
  {
    table_layout l1 = {1, 2}, l2 = {1, 2};
    auto permutation = id_permutation(2);
    table<int> tab1(2, {{1, 2}}), tab2(2, {{1, 2}, {3, 4}}),
      tab3(2, {{3, 4}, {1, 2}});
    EXPECT_TRUE(tab1.t_union(tab2, permutation).equal_to(tab3, permutation));
    tab1.t_union_in_place(tab2, permutation);
    EXPECT_TRUE(tab1.equal_to(tab3, permutation));
  }
  {
    table_layout l1 = {2, 3, 1}, l2 = {3, 2, 1}, l3 = {1, 2, 3};
    table<int> tab1(3, {{4, 5, 6}, {7, 8, 9}}), tab2(3, {{7, 6, 5}, {3, 2, 7}}),
      tab3(3, {{6, 4, 5}, {7, 2, 3}, {9, 7, 8}, {5, 6, 7}});
    auto permutation1 = find_permutation(l1, l2),
         permutation2 = find_permutation(l1, l3);
    EXPECT_TRUE(tab1.t_union(tab2, permutation1).equal_to(tab3, permutation2));
    tab1.t_union_in_place(tab2, permutation1);
    EXPECT_TRUE(tab1.equal_to(tab3, permutation2));
  }
  {
    table_layout l1 = {2, 3, 1}, l2 = {3, 2, 1}, l3 = {1, 2, 3};
    table<int> tab1(3, {{4, 5, 6}, {7, 8, 9}}), tab2(3, {{7, 6, 5}, {3, 2, 7}}),
      tab3(3, {{6, 4, 5}, {7, 3, 2}, {9, 7, 8}, {5, 6, 7}});
    auto permutation1 = find_permutation(l1, l2),
         permutation2 = find_permutation(l1, l3);
    EXPECT_FALSE(tab1.t_union(tab2, permutation1).equal_to(tab3, permutation2));
    tab1.t_union_in_place(tab2, permutation1);
    EXPECT_FALSE(tab1.equal_to(tab3, permutation2));
  }
  {
    table_layout l1 = {5, 20, 11, 9}, l2 = {11, 9, 20, 5}, l3 = {5, 20, 11, 9};
    auto permutation1 = find_permutation(l1, l2),
         permutation2 = find_permutation(l1, l3);
    table<int> tab1(4, {{1, 2, 3, 4}, {5, 6, 7, 8}, {9, 10, 11, 12}}),
      tab2(4, {{13, 14, 15, 16}, {17, 18, 19, 20}, {21, 22, 23, 24}}),
      tab3(4, {{5, 6, 7, 8},
               {1, 2, 3, 4},
               {16, 15, 13, 14},
               {20, 19, 17, 18},
               {9, 10, 11, 12},
               {24, 23, 21, 22}});
    EXPECT_TRUE(tab1.t_union(tab2, permutation1).equal_to(tab3, permutation2));
    tab1.t_union_in_place(tab2, permutation1);
    EXPECT_TRUE(tab1.equal_to(tab3, permutation2));
  }
}

// TODO: add test cases for anti join