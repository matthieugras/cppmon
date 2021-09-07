#include <fmt/format.h>
#include <table.h>

void equality_check() { 
  table<int> tab1({1, 2, 3}, {{5, 6, 7}, {8, 9, 10}}),
             tab2({1, 2, 4}, {{5, 6, 7}, {8, 9, 10}});
  fmt::print("{}", tab1);
}

int main() {
    equality_check();
}