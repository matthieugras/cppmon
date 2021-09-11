#include <mformula.h>
#include <formula.h>
#include <fmt/core.h>

int main() {
  auto pred = fo::Pred{"rofl", vector<fo::Term>()};
  auto form = fo::Formula{pred};
  form.is_safe();
  fmt::print("{}\n", form.comp_fv().size());
}