#include <fmt/core.h>
#include <fmt/format.h>
#include "table.h"

int main() {
    fmt::print(FMT_STRING("roflcopter {}\n"), "roflcopter");
    table<int> tab({1,2,3});
    table<int> tab2({5,6,7});
    auto tab3 = tab.natural_join(tab2);
    return 0;
}
