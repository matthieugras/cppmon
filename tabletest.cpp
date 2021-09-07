#include <fmt/format.h>
#include <table.h>

void equality_check() {
  table<int> tab1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      tab2({1, 2, 4}, {{5, 6, 7}, {8, 9, 10}});
  assert(tab1 != tab2);
  tab1 = table<int>({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
  tab2 = table<int>({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
  assert(tab1 == tab2);
  tab1 = table<int>({}, {{}, {}, {}});
  tab2 = table<int>({}, {});
  assert(tab1 == tab2);
  tab1 = table<int>({0, 1}, {{1, 2}, {1, 2}});
  tab2 = table<int>({0, 1}, {{1, 2}});
  assert(tab1 == tab2);
  tab1 = table<int>({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
  tab2 = table<int>({3, 2, 1}, {{7, 6, 5}, {10, 9, 8}});
  assert(tab1 == tab2);
  tab1 = table<int>({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
  tab2 = table<int>({3, 2, 1}, {{7, 6, 5}, {9, 9, 8}});
  assert(tab1 != tab2);
  tab1 = table<int>({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}, {11, 12, 13}});
  tab2 = table<int>({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
  assert(tab1 != tab2);
}

void table_joins() {
  table<int> tab1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
      tab2({4, 5, 6}, {{10, 20, 30}, {4, 2, 1}}),
      tab_res({1, 2, 3, 4, 5, 6}, {
                                      {5, 6, 7, 10, 20, 30},
                                      {5, 6, 7, 4, 2, 1},
                                      {8, 9, 10, 10, 20, 30},
                                      {8, 9, 10, 4, 2, 1},
                                  });
  assert(tab1.natural_join(tab2) == tab_res);
  tab1 = table<int>({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
  tab2 = table<int>({2, 5, 6}, {{10, 20, 30}, {4, 2, 1}}),
  tab_res = table<int>({1, 2, 3, 5, 6}, {});
  assert(tab1.natural_join(tab2) == tab_res);
  tab1 = table<int>({}, {{}, {}, {}});
  tab2 = table<int>({}, {{}, {}}), tab_res = table<int>({}, {});
  assert(tab1.natural_join(tab2) == tab_res);
  tab1 = table<int>({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}});
  tab2 = table<int>({2, 5, 6}, {{10, 20, 30}, {6, 2, 1}}),
  tab_res = table<int>({1, 2, 3, 5, 6}, {{5, 6, 7, 2, 1}});
  assert(tab1.natural_join(tab2) == tab_res);
  tab1 = table<int>({6, 2, 3}, {{5, 6, 7}, {5, 9, 10}});
  tab2 = table<int>({4, 5, 6}, {{10, 20, 5}, {11, 2, 22}}),
  tab_res =
      table<int>({2, 3, 4, 5, 6}, {{6, 7, 10, 20, 5}, {9, 10, 10, 20, 5}});
  assert(tab1.natural_join(tab2) == tab_res);
  tab1 = table<int>({1, 2, 6, 4, 5},
                    {{1, 2, 3, 4, 5}, {6, 7, 8, 9, 10}, {11, 23, 13, 14, 24}});
  tab2 = table<int>({3, 2, 5},
                    {{16, 2, 5}, {19, 2, 5}, {22, 23, 24}, {25, 26, 27}}),
  tab_res = table<int>(
      {1, 2, 3, 4, 5, 6},
      {{1, 2, 16, 4, 5, 3}, {1, 2, 19, 4, 5, 3}, {11, 23, 22, 14, 24, 13}});
  assert(tab1.natural_join(tab2) == tab_res);
}

int main() {
  equality_check();
  table_joins();
}