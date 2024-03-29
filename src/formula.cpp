#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <formula.h>
#include <stdexcept>
#include <type_traits>
#include <util.h>
#include <utility>

namespace fo {
using std::literals::string_view_literals::operator""sv;

agg_type agg_type_from_json(const json &json_formula) {
  string_view agg_ty = json_formula.at(0).get<string_view>();
  if (agg_ty == "Agg_Cnt"sv) {
    return agg_type::CNT;
  } else if (agg_ty == "Agg_Min"sv) {
    return agg_type::MIN;
  } else if (agg_ty == "Agg_Max"sv) {
    return agg_type::MAX;
  } else if (agg_ty == "Agg_Sum"sv) {
    return agg_type::SUM;
  } else if (agg_ty == "Agg_Avg"sv) {
    return agg_type::AVG;
  } else if (agg_ty == "Agg_Med"sv) {
    return agg_type::MED;
  } else {
    throw std::runtime_error("invalid aggregation type");
  }
}

size_t nat_from_json(const json &json_formula) {
  string_view nat_ty = json_formula.at(0).get<string_view>();
  if (nat_ty == "Nat"sv) {
    return json_formula.at(1).get<size_t>();
  } else {
    throw std::runtime_error("invalid nat json");
  }
}

optional<size_t> enat_from_json(const json &json_formula) {
  string_view enat_ty = json_formula.at(0).get<string_view>();
  if (enat_ty == "Enat"sv) {
    return nat_from_json(json_formula.at(1));
  } else if (enat_ty == "Infinity_enat"sv) {
    return {};
  } else {
    throw std::runtime_error("invalid enat json");
  }
}

// Interval member function
Interval::Interval(size_t lower_bound, size_t upper_bound, bool bounded)
    : l_(lower_bound), u_(upper_bound), bounded_(bounded) {
#ifndef NDEBUG
  if (bounded && l_ > u_)
    throw std::invalid_argument("must have l <= u");
#endif
}

bool Interval::operator==(const Interval &other) const {
  if (bounded_ != other.bounded_)
    return false;
  if (bounded_)
    return l_ == other.l_ && u_ == other.u_;
  else
    return l_ == other.l_;
}

size_t Interval::get_lower() const { return l_; }

bool Interval::geq_lower(size_t n) const { return n >= l_; }

bool Interval::lt_lower(size_t n) const { return n < l_; }

bool Interval::leq_upper(size_t n) const {
  if (!is_bounded())
    return true;
  else
    return n <= u_;
}

bool Interval::gt_upper(size_t n) const {
  if (!is_bounded())
    return false;
  else
    return n > u_;
}

Interval Interval::from_json(const json &json_formula) {
  size_t l = nat_from_json(json_formula.at(0));
  optional<size_t> u = enat_from_json(json_formula.at(1));
  if (u)
    return {l, u.value(), true};
  else
    return {l, 0, false};
}

bool Interval::contains(size_t n) const {
  if (!bounded_)
    return n >= l_;
  else
    return n >= l_ && n <= u_;
}

bool Interval::is_bounded() const { return bounded_; }

// Term member functions
Term::Term(const Term &t) : val(copy_val(t.val)) {}

Term::Term(val_type &&val) noexcept : val(std::move(val)) {}
Term::Term(const val_type &val) : val(copy_val(val)) {}

event_data Term::eval(const vector<size_t> &var_2_idx,
                      const vector<event_data> &tuple) const {
  auto visitor = [&var_2_idx, &tuple](auto &&arg) -> event_data {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, event_data>) {
      return arg;
    } else if constexpr (std::is_same_v<T, var_t>) {
      return tuple[var_2_idx[arg.idx]];
    } else if constexpr (std::is_same_v<T, plus_t>) {
      return arg.l->eval(var_2_idx, tuple) + arg.r->eval(var_2_idx, tuple);
    } else if constexpr (std::is_same_v<T, minus_t>) {
      return arg.l->eval(var_2_idx, tuple) - arg.r->eval(var_2_idx, tuple);
    } else if constexpr (std::is_same_v<T, uminus_t>) {
      return -arg.t->eval(var_2_idx, tuple);
    } else if constexpr (std::is_same_v<T, mult_t>) {
      return arg.l->eval(var_2_idx, tuple) * arg.r->eval(var_2_idx, tuple);
    } else if constexpr (std::is_same_v<T, div_t>) {
      return arg.l->eval(var_2_idx, tuple) / arg.r->eval(var_2_idx, tuple);
    } else if constexpr (std::is_same_v<T, mod_t>) {
      return arg.l->eval(var_2_idx, tuple) % arg.r->eval(var_2_idx, tuple);
    } else if constexpr (std::is_same_v<T, f2i_t>) {
      return arg.t->eval(var_2_idx, tuple).float_to_int();
    } else if constexpr (std::is_same_v<T, i2f_t>) {
      return arg.t->eval(var_2_idx, tuple).int_to_float();
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}

bool Term::operator==(const Term &other) const {
  auto visitor = [](auto &&arg1, auto &&arg2) -> bool {
    using T1 = std::decay_t<decltype(arg1)>;
    using T2 = std::decay_t<decltype(arg2)>;
    if constexpr (!std::is_same_v<T1, T2>) {
      return false;
    } else if constexpr (std::is_same_v<T1, var_t>) {
      return arg1.idx == arg2.idx;
    } else if constexpr (std::is_same_v<T1, event_data>) {
      return arg1 == arg2;
    } else if constexpr (any_type_equal_v<T1, uminus_t, f2i_t, i2f_t>) {
      return (*arg1.t) == (*arg2.t);
    } else if constexpr (any_type_equal_v<T1, plus_t, minus_t, mult_t, div_t,
                                          mod_t>) {
      return ((*arg1.l) == (*arg2.l)) && ((*arg1.r) == (*arg2.r));
    } else {
      static_assert(always_false_v<T1>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val, other.val);
}

Term Term::from_json(const json &json_formula) {
  string_view term_ty = json_formula.at(0).get<string_view>();
  if (term_ty == "Var"sv) {
    return Term::Var(nat_from_json(json_formula.at(1)));
  } else if (term_ty == "Const"sv) {
    return Term::Const(event_data::from_json(json_formula.at(1)));
  } else if (term_ty == "Plus"sv) {
    return Term::Plus(Term::from_json(json_formula.at(1)),
                      Term::from_json(json_formula.at(2)));
  } else if (term_ty == "Minus"sv) {
    return Term::Minus(Term::from_json(json_formula.at(1)),
                       Term::from_json(json_formula.at(2)));
  } else if (term_ty == "Mult"sv) {
    return Term::Mult(Term::from_json(json_formula.at(1)),
                      Term::from_json(json_formula.at(2)));
  } else if (term_ty == "Div"sv) {
    return Term::Div(Term::from_json(json_formula.at(1)),
                     Term::from_json(json_formula.at(2)));
  } else if (term_ty == "Mod"sv) {
    return Term::Mod(Term::from_json(json_formula.at(1)),
                     Term::from_json(json_formula.at(2)));
  } else if (term_ty == "F2i"sv) {
    return Term::F2i(Term::from_json(json_formula.at(1)));
  } else if (term_ty == "I2f"sv) {
    return Term::I2f(Term::from_json(json_formula.at(1)));
  } else {
    throw std::runtime_error("invalid term json");
  }
}

Term::val_type Term::copy_val(const val_type &val) {
  auto visitor = [](auto &&arg) -> val_type {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, event_data, var_t>) {
      return arg;
    } else if constexpr (any_type_equal_v<T, uminus_t, f2i_t, i2f_t>) {
      return T{uniq(copy_val(arg.t->val))};
    } else if constexpr (any_type_equal_v<T, plus_t, minus_t, mult_t, div_t,
                                          mod_t>) {
      return T{uniq(copy_val(arg.l->val)), uniq(copy_val(arg.r->val))};
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}

Term Term::Var(size_t idx) { return Term(var_t{idx}); }
Term Term::Const(event_data c) { return Term(std::move(c)); }
Term Term::Plus(Term l, Term r) {
  return Term(plus_t{uniq(std::move(l)), uniq(std::move(r))});
}
Term Term::Minus(Term l, Term r) {
  return Term(minus_t{uniq(std::move(l)), uniq(std::move(r))});
}
Term Term::UMinus(Term t) { return Term(uminus_t{uniq(std::move(t))}); }
Term Term::Mult(Term l, Term r) {
  return Term(mult_t{uniq(std::move(l)), uniq(std::move(r))});
}
Term Term::Div(Term l, Term r) {
  return Term(div_t{uniq(std::move(l)), uniq(std::move(r))});
}
Term Term::Mod(Term l, Term r) {
  return Term(mod_t{uniq(std::move(l)), uniq(std::move(r))});
}
Term Term::F2i(Term t) { return Term(f2i_t{uniq(std::move(t))}); }
Term Term::I2f(Term t) { return Term(i2f_t{uniq(std::move(t))}); }

bool Term::is_const() const { return var2::holds_alternative<event_data>(val); }
bool Term::is_var() const { return var2::holds_alternative<var_t>(val); }
const size_t *Term::get_if_var() const {
  if (const auto *ptr = var2::get_if<var_t>(&val)) {
    return &(ptr->idx);
  } else {
    return nullptr;
  }
}

const event_data *Term::get_if_const() const {
  return var2::get_if<event_data>(&val);
}

fv_set fvi_single_var(size_t var, size_t num_bound_vars) {
  if (var >= num_bound_vars)
    return fv_set({var - num_bound_vars});
  else
    return {};
}

fv_set Term::fvi(size_t num_bound_vars) const {
  auto visitor = [num_bound_vars](auto &&arg) -> fv_set {
    using T = std::decay_t<decltype(arg)>;

    if constexpr (std::is_same_v<T, var_t>) {
      return fvi_single_var(arg.idx, num_bound_vars);
    } else if constexpr (any_type_equal_v<T, plus_t, minus_t, mult_t, div_t,
                                          mod_t>) {
      auto fvs1 = arg.l->fvi(num_bound_vars), fvs2 = arg.r->fvi(num_bound_vars);
      return hash_set_union(fvs1, fvs2);
    } else if constexpr (any_type_equal_v<T, uminus_t, f2i_t, i2f_t>) {
      return arg.t->fvi(num_bound_vars);
    } else if constexpr (std::is_same_v<T, event_data>) {
      return {};
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}
fv_set Term::fvs() const { return fvi(0); }

// Formula member functions
size_t Formula::formula_id_counter = 0;
std::uint32_t Formula::pred_id_counter = USER_PRED;
pred_map_t Formula::known_preds = pred_map_t{};

size_t Formula::unique_id() const { return formula_id_; }

Formula::Formula(const Formula &formula)
    : formula_id_(formula_id_counter), val(copy_val(formula.val)) {
  formula_id_counter++;
}
Formula::Formula(Formula &&formula) noexcept
    : formula_id_(formula_id_counter), val(std::move(formula.val)) {
  formula_id_counter++;
}
Formula::Formula(string_view json_formula)
    : Formula(Formula::from_json(json::parse(json_formula))) {}

Formula *Formula::inner_if_neg() const {
  if (auto *ptr = var2::get_if<neg_t>(&val)) {
    return ptr->phi.get();
  }
  return nullptr;
}

bool Formula::operator==(const Formula &other) const {
  auto visitor = [](auto &&arg1, auto &&arg2) -> bool {
    using T1 = std::decay_t<decltype(arg1)>;
    using T2 = std::decay_t<decltype(arg2)>;
    if constexpr (!std::is_same_v<T1, T2>) {
      return false;
    } else if constexpr (std::is_same_v<T1, pred_t>) {
      return (arg1.pred_id == arg2.pred_id) &&
             (arg1.pred_args == arg2.pred_args);
    } else if constexpr (any_type_equal_v<T1, eq_t, less_t, less_eq_t>) {
      return (arg1.l == arg2.l) && (arg1.r == arg2.r);
    } else if constexpr (any_type_equal_v<T1, neg_t, exists_t>) {
      return (*arg1.phi) == (*arg2.phi);
    } else if constexpr (any_type_equal_v<T1, or_t, and_t>) {
      return ((*arg1.phil) == (*arg2.phil)) && ((*arg1.phir) == (*arg2.phir));
    } else if constexpr (any_type_equal_v<T1, prev_t, next_t>) {
      return (arg1.inter == arg2.inter) && ((*arg1.phi) == (*arg2.phi));
    } else if constexpr (any_type_equal_v<T1, since_t, until_t>) {
      return (arg1.inter == arg2.inter) && ((*arg1.phil) == (*arg2.phil)) &&
             ((*arg1.phir) == (*arg2.phir));
    } else if constexpr (std::is_same_v<T1, agg_t>) {
      return (arg1.ty == arg2.ty) && (arg1.res_var == arg2.res_var) &&
             (arg1.num_bound_vars == arg2.num_bound_vars) &&
             (arg1.default_value == arg2.default_value) &&
             (arg1.agg_term == arg2.agg_term) && ((*arg1.phi) == (*arg2.phi));
    } else if constexpr (std::is_same_v<T1, let_t>) {
      return (arg1.pred_id == arg2.pred_id) && ((*arg1.phi) == (*arg2.phi)) &&
             ((*arg1.psi) == (*arg2.psi));
    } else {
      static_assert(always_false_v<T1>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val, other.val);
}

Formula Formula::from_json(const json &json_formula) {
  if (!json_formula.is_array())
    throw std::runtime_error("bad json - should be array");
  string_view fo_ty = json_formula.at(0).get<string_view>();
  if (fo_ty == "Pred"sv) {
    string pred_name = json_formula.at(1).get<string>();
    vector<Term> trm_list;
    std::transform(json_formula.at(2).cbegin(), json_formula.at(2).cend(),
                   std::back_inserter(trm_list),
                   [](const json &j) -> Term { return Term::from_json(j); });
    return Pred(std::move(pred_name), std::move(trm_list), false);
  } else if (fo_ty == "Eq"sv) {
    return Eq(Term::from_json(json_formula.at(1)),
              Term::from_json(json_formula.at(2)));
  } else if (fo_ty == "Less"sv) {
    return Less(Term::from_json(json_formula.at(1)),
                Term::from_json(json_formula.at(2)));
  } else if (fo_ty == "LessEq"sv) {
    return LessEq(Term::from_json(json_formula.at(1)),
                  Term::from_json(json_formula.at(2)));
  } else if (fo_ty == "Neg"sv) {
    return Neg(Formula::from_json(json_formula.at(1)));
  } else if (fo_ty == "Or"sv) {
    return Or(Formula::from_json(json_formula.at(1)),
              Formula::from_json(json_formula.at(2)));
  } else if (fo_ty == "And"sv) {
    return And(Formula::from_json(json_formula.at(1)),
               Formula::from_json(json_formula.at(2)));
  } else if (fo_ty == "Exists"sv) {
    return Formula::Exists(Formula::from_json(json_formula.at(1)));
  } else if (fo_ty == "Prev"sv) {
    return Prev(Interval::from_json(json_formula.at(1)),
                Formula::from_json(json_formula.at(2)));
  } else if (fo_ty == "Next"sv) {
    return Next(Interval::from_json(json_formula.at(1)),
                Formula::from_json(json_formula.at(2)));
  } else if (fo_ty == "Since"sv) {
    return Since(Interval::from_json(json_formula.at(2)),
                 Formula::from_json(json_formula.at(1)),
                 Formula::from_json(json_formula.at(3)));
  } else if (fo_ty == "Until"sv) {
    return Until(Interval::from_json(json_formula.at(2)),
                 Formula::from_json(json_formula.at(1)),
                 Formula::from_json(json_formula.at(3)));
  } else if (fo_ty == "Agg"sv) {
    auto res_var = nat_from_json(json_formula.at(1));
    auto ty = agg_type_from_json(json_formula.at(2).at(0));
    auto default_value = event_data::from_json(json_formula.at(2).at(1));
    auto num_bound_vars = nat_from_json(json_formula.at(3));
    auto agg_term = Term::from_json(json_formula.at(4));
    auto phi = Formula::from_json(json_formula.at(5));
    return Formula::Agg(ty, res_var, num_bound_vars, std::move(default_value),
                        std::move(agg_term), std::move(phi));
  } else if (fo_ty == "Let"sv) {
    auto pred_name = json_formula.at(1).get<std::string>();
    auto phi = Formula::from_json(json_formula.at(2));
    auto psi = Formula::from_json(json_formula.at(3));
    return Formula::Let(std::move(pred_name), std::move(phi), std::move(psi));
  } else if (fo_ty == "TP"sv) {
    std::string pred_name = "tp";
    return Pred(pred_name, make_vector(Term::from_json(json_formula.at(1))),
                true);
  } else if (fo_ty == "TS"sv) {
    std::string pred_name = "ts";
    return Pred(pred_name, make_vector(Term::from_json(json_formula.at(1))),
                true);
  } else if (fo_ty == "TPTS"sv) {
    std::string pred_name = "tpts";
    return Pred(pred_name,
                make_vector(Term::from_json(json_formula.at(1)),
                            Term::from_json(json_formula.at(2))),
                true);
  } else {
    throw std::runtime_error("formula type not supported");
  }
}

Formula::Formula(val_type &&val) noexcept
    : formula_id_(formula_id_counter), val(std::move(val)) {
  formula_id_counter++;
}

Formula::Formula(const val_type &val) : Formula(copy_val(val)) {}

const pred_map_t &Formula::get_known_preds() { return known_preds; }
pred_id_t Formula::add_pred_to_map(std::string pred_name, size_t arity) {
  std::uint32_t pred_id;
  auto key = std::pair(pred_name, arity);
  if (pred_name == "tp" && arity == 1)
    pred_id = TP_PRED;
  else if (pred_name == "ts" && arity == 1)
    pred_id = TS_PRED;
  else if (pred_name == "tpts" && arity == 2)
    pred_id = TP_TS_PRED;
  else {
    auto p_it = known_preds.find(key);
    if (p_it == known_preds.end()) {
      pred_id = pred_id_counter++;
    } else {
      pred_id = p_it->second;
    }
  }
  known_preds.emplace(key, pred_id);
  return pred_id;
}

Formula Formula::Pred(name pred_name, vector<Term> pred_args, bool is_builtin) {
  size_t n_args = pred_args.size();
  auto pred_id = Formula::add_pred_to_map(std::move(pred_name), n_args);
  return Formula(pred_t{pred_id, std::move(pred_args), is_builtin});
}

Formula Formula::Eq(Term l, Term r) {
  return Formula(eq_t{std::move(l), std::move(r)});
}
Formula Formula::Less(Term l, Term r) {
  return Formula(less_t{std::move(l), std::move(r)});
}
Formula Formula::LessEq(Term l, Term r) {
  return Formula(less_eq_t{std::move(l), std::move(r)});
}
Formula Formula::Neg(Formula phi) {
  return Formula(neg_t{uniq(std::move(phi))});
}
Formula Formula::Or(Formula phil, Formula phir) {
  return Formula(or_t{uniq(std::move(phil)), uniq(std::move(phir))});
}
Formula Formula::And(Formula phil, Formula phir) {
  return Formula(and_t{uniq(std::move(phil)), uniq(std::move(phir))});
}
Formula Formula::Exists(Formula phi) {
  return Formula(Formula::exists_t{uniq(std::move(phi))});
}
Formula Formula::Prev(Interval inter, Formula phi) {
  return Formula(prev_t{inter, uniq(std::move(phi))});
}
Formula Formula::Next(Interval inter, Formula phi) {
  return Formula(next_t{inter, uniq(std::move(phi))});
}
Formula Formula::Since(Interval inter, Formula phil, Formula phir) {
  return Formula(since_t{inter, uniq(std::move(phil)), uniq(std::move(phir))});
}
Formula Formula::Until(Interval inter, Formula phil, Formula phir) {
  return Formula(until_t{inter, uniq(std::move(phil)), uniq(std::move(phir))});
}
Formula Formula::Agg(agg_type ty, size_t res_var, size_t num_bound_vars,
                     event_data default_value, Term agg_term, Formula phi) {
  return Formula(agg_t{ty, res_var, num_bound_vars, std::move(default_value),
                       std::move(agg_term), uniq(std::move(phi))});
}

Formula Formula::Let(std::string pred_name, Formula phi, Formula psi) {
  size_t arity = phi.fvs().size();
  auto pred_id = Formula::add_pred_to_map(std::move(pred_name), arity);
  return Formula(let_t{pred_id, uniq(std::move(phi)), uniq(std::move(psi))});
}

Formula::val_type Formula::copy_val(const val_type &val) {
  auto visitor = [](auto &&arg) -> val_type {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, pred_t, eq_t, less_t, less_eq_t>) {
      return arg;
    } else if constexpr (any_type_equal_v<T, neg_t, exists_t>) {
      return T{uniq(copy_val(arg.phi->val))};
    } else if constexpr (any_type_equal_v<T, or_t, and_t>) {
      return T{uniq(copy_val(arg.phil->val)), uniq(copy_val(arg.phir->val))};
    } else if constexpr (any_type_equal_v<T, prev_t, next_t>) {
      return T{arg.inter, uniq(copy_val(arg.phi->val))};
    } else if constexpr (any_type_equal_v<T, since_t, until_t>) {
      return T{arg.inter, uniq(copy_val(arg.phil->val)),
               uniq(copy_val(arg.phir->val))};
    } else if constexpr (std::is_same_v<T, agg_t>) {
      return T{arg.ty,
               arg.res_var,
               arg.num_bound_vars,
               arg.default_value,
               arg.agg_term,
               uniq(copy_val(arg.phi->val))};
    } else if constexpr (std::is_same_v<T, let_t>) {
      return T{arg.pred_id, uniq(copy_val(arg.phi->val)),
               uniq(copy_val(arg.psi->val))};
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}

fv_set Formula::fvi(size_t num_bound_vars) const {
  auto visitor = [num_bound_vars](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    using std::is_same_v;
    if constexpr (is_same_v<T, pred_t>) {
      fv_set fv_children;
      for (const auto &t : arg.pred_args) {
        auto child_set = t.fvi(num_bound_vars);
        fv_children.insert(child_set.begin(), child_set.end());
      }
      return fv_children;
    } else if constexpr (any_type_equal_v<T, less_t, less_eq_t, eq_t>) {
      auto fvl = arg.l.fvi(num_bound_vars), fvr = arg.r.fvi(num_bound_vars);
      return hash_set_union(fvl, fvr);
    } else if constexpr (any_type_equal_v<T, prev_t, next_t, neg_t>) {
      return arg.phi->fvi(num_bound_vars);
    } else if constexpr (any_type_equal_v<T, and_t, or_t, since_t, until_t>) {
      auto fvl = arg.phil->fvi(num_bound_vars),
           fvr = arg.phir->fvi(num_bound_vars);
      return hash_set_union(fvl, fvr);
    } else if constexpr (is_same_v<T, exists_t>) {
      return arg.phi->fvi(num_bound_vars + 1);
    } else if constexpr (is_same_v<T, agg_t>) {
      size_t new_num_bound_vars = num_bound_vars + arg.num_bound_vars;
      auto fv_trm = arg.agg_term.fvi(new_num_bound_vars),
           fv_phi = arg.phi->fvi(new_num_bound_vars),
           fv_res_var = fvi_single_var(arg.res_var, num_bound_vars);
      return hash_set_union(fv_trm, fv_phi, fv_res_var);
    } else if constexpr (is_same_v<T, let_t>) {
      return arg.psi->fvi(num_bound_vars);
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}

fv_set Formula::fvs() const { return fvi(0); }

size_t Formula::degree() const {
  auto this_fvs = fvs();
  return this_fvs.size();
}

bool Formula::is_always_true() const {
  if (const auto *ptr = var2::get_if<fo::Formula::eq_t>(&val)) {
    const auto *l_val = ptr->l.get_if_const(), *r_val = ptr->r.get_if_const();
    if (l_val && r_val) {
      const auto *l_data = l_val->get_if_int(), *r_data = r_val->get_if_int();
      if (l_data && r_data && *l_data == 0 && *r_data == 0)
        return true;
    }
  }
  return false;
}

bool Formula::is_constraint() const {
  auto visitor = [](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, eq_t, less_t, less_eq_t>) {
      return true;
    } else if constexpr (std::is_same_v<T, neg_t>) {
      return arg.phi->is_constraint();
    } else {
      return false;
    }
  };
  return var2::visit(visitor, val);
}

bool Formula::is_safe_assignment(const fv_set &vars) const {
  using var2::get;
  if (const eq_t *ptr = var2::get_if<eq_t>(&val)) {
    const auto &t1 = ptr->l, &t2 = ptr->r;
    const size_t *var1 = t1.get_if_var(), *var2 = t2.get_if_var();
    if (var1 && var2) {
      return vars.contains(*var1) != vars.contains(*var2);
    } else if (var1) {
      return (!vars.contains(*var1)) && is_subset(t2.fvs(), vars);
    } else if (var2) {
      return (!vars.contains(*var2) && is_subset(t1.fvs(), vars));
    }
  }
  return false;
}

bool Formula::is_safe_formula() const {
  using std::is_same_v;
  using var2::get;
  auto visitor = [](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (is_same_v<T, eq_t>) {
      const auto &[l, r] = arg;
      return (l.is_const() && (r.is_const() || r.is_var())) ||
             (l.is_var() && r.is_const());
    } else if constexpr (any_type_equal_v<T, less_t, less_eq_t>) {
      return false;
    } else if constexpr (is_same_v<T, neg_t>) {
      if (const auto *ptr = var2::get_if<eq_t>(&arg.phi->val)) {
        const auto *var1 = ptr->l.get_if_var(), *var2 = ptr->r.get_if_var();
        if (var1 && var2)
          return (*var1) == (*var2);
      }
      return arg.phi->fvs().empty() && arg.phi->is_safe_formula();
    } else if constexpr (is_same_v<T, pred_t>) {
      return std::all_of(
        arg.pred_args.cbegin(), arg.pred_args.cend(),
        [](const Term &t) { return t.is_var() || t.is_const(); });
    } else if constexpr (is_same_v<T, or_t>) {
      const auto &[phil, phir] = arg;
      auto fvl = phil->fvs(), fvr = phir->fvs();
      if (fvl != fvr)
        return false;
      return phil->is_safe_formula() && phir->is_safe_formula();
    } else if constexpr (is_same_v<T, and_t>) {
      const auto &[phil, phir] = arg;
      if (!phil->is_safe_formula())
        return false;
      if (phir->is_safe_assignment(phil->fvs()) || phir->is_safe_formula())
        return true;
      if (!is_subset(phir->fvs(), phil->fvs()))
        return false;
      const auto *neg_ptr = var2::get_if<neg_t>(&phir->val);
      return phir->is_constraint() ||
             (neg_ptr && neg_ptr->phi->is_safe_formula());
    } else if constexpr (any_type_equal_v<T, exists_t, prev_t, next_t>) {
      return arg.phi->is_safe_formula();
    } else if constexpr (any_type_equal_v<T, since_t, until_t>) {
      const auto &phil = *arg.phil, &phir = *arg.phir;
      if (is_subset(phil.fvs(), phir.fvs()) && phil.is_safe_formula())
        return true;
      if (!arg.phir->is_safe_formula())
        return false;
      if (const auto *neg_ptr = phil.inner_if_neg()) {
        return neg_ptr->is_safe_formula();
      }
      return false;
    } else if constexpr (is_same_v<T, agg_t>) {
      auto fv_agg_trm = arg.agg_term.fvs(), fv_phi = arg.phi->fvs();
      bool safe_tmp = arg.phi->is_safe_formula() &&
                      !fv_phi.contains(arg.res_var + arg.num_bound_vars) &&
                      is_subset(fv_agg_trm, fv_phi);
      for (size_t var = 0; var < arg.num_bound_vars; ++var)
        safe_tmp = safe_tmp && fv_phi.contains(var);
      return safe_tmp;
    } else if constexpr (is_same_v<T, let_t>) {
      auto fv_phi = arg.phi->fvs();
      bool safe_tmp = arg.phi->is_safe_formula() && arg.psi->is_safe_formula();
      for (size_t var = 0; var < fv_phi.size(); ++var)
        safe_tmp = safe_tmp && fv_phi.contains(var);
      return safe_tmp;
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}
}// namespace fo
