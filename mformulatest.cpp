#include <mformula.h>
#include <formula.h>
#include <fmt/core.h>
#include <vector>

int main() {
  using std::vector;
  auto pred = fo::Pred{"rofl", vector<fo::Term>()};
  fo::Formula form = fo::Formula(pred);
  fmt::print("{}", form.is_safe_formula());
  fmt::print("{}\n", form.comp_fv().size());
}