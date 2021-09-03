#include "boost/hana/fwd/tuple.hpp"
#include <boost/hana.hpp>
#include <formula.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <variant>

namespace fo {
string to_string(const std::string &s) {
  return quote(s);
}

std::string quote(std::string s) { return "\"" + s + "\""; }
string to_string(const fo::agg_op &o) {
  switch (o) {
  case Cnt:
    return "Cnt";
  case Min:
    return "Min";
  case Max:
    return "Max";
  case Sum:
    return "Sum";
  case Avg:
    return "Avg";
  case Med:
    return "Med";
  default:
    throw std::runtime_error("bad enum val");
  }
}

string to_string(const tcst &o) {
  switch (o) {
  case TInt:
    return "TInt";
  case TStr:
    return "TStr";
  case TFloat:
    return "TFloat";
  default:
    throw std::runtime_error("bad enum val");
  }
}

string to_string(const tcl &o) {
  switch (o) {
  case TNum:
    return "TNum";
  case TAny:
    return "TAny";
  default:
    throw std::runtime_error("bad enum val");
  }
}
} // namespace fo

/*
int main() {
  namespace hana = boost::hana;
  using std::cout;
  using std::endl;
  using std::make_unique;
  using std::move;
  auto bla = fo::Var("504584858485845848584858458485884888888888888888888888888888");
  fo::term bla2(bla);
  cout << fo::to_string(bla) << endl;
  cout << fo::to_string(bla2) << endl;
  return 0;
}*/
