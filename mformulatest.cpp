#include <mformula.h>
#include <formula.h>
#include <fmt/core.h>

int main() {
  auto pred = fo::Pred{"rofl", vector<fo::Term>()};
  auto form = fo::Formula{pred};
  fmt::print("{}", form.is_safe_formula());
  fmt::print("{}\n", form.comp_fv().size());
}