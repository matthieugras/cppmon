#ifndef MFORMULA_H
#define MFORMULA_H

#include <table.h>
#include <formula.h>
#include <vector>
#include <absl/container/flat_hash_set.h>

using std::vector;

struct MFormula {

};

struct MPred {
  fo::name pred_name;
  vector<fo::Term> terms;
};

struct MAnd {
  vector<table<fo::event_data>> state_l, state_r;
  MFormula phi_l, phi_r;
  bool is_anti_join;
};

#endif