#ifndef MFORMULA_H
#define MFORMULA_H

#include <absl/container/flat_hash_set.h>
#include <formula.h>
#include <table.h>
#include <vector>

namespace mfo {
namespace detail {
  using fo::event_data;
  using fo::Formula;
  using fo::Term;
  using fo::detail::ptr_type;
  using fo::name;
  using std::vector;

  struct MFormula;

  struct MRel {
    table<event_data> tab;
  };

  struct MPred {
    name pred_name;
    vector<Term> pred_args;
  };

  struct MAnd {

  };

  struct MAndAssign {};

  struct MAndRel {};

  struct MOr {};

  struct MNeg {};

  struct MExists {};

  struct MPrev {};

  struct MNext {};

  struct MSince {};

  struct MUntil {};

}// namespace detail

using detail::MAnd;
using detail::MAndAssign;
using detail::MAndRel;
using detail::MExists;
using detail::MFormula;
using detail::MNeg;
using detail::MNext;
using detail::MOr;
using detail::MPred;
using detail::MPrev;
using detail::MRel;
using detail::MSince;
using detail::MUntil;

}// namespace mfo


#endif