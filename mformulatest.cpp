#include <mformula.h>
#include <formula.h>
#include <fmt/core.h>
#include <vector>

int main() {
  using std::vector;
  auto pred = fo::Pred{"rofl", vector<fo::Term>()};
  fo::Formula form(pred);
  //mfo::MState state(form);
  fmt::print("{}", form.is_monitorable());
  fmt::print("{}\n", form.comp_fv().size());
}