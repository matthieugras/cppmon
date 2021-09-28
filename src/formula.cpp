#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <formula.h>
#include <stdexcept>
#include <type_traits>
#include <util.h>
#include <utility>

namespace fo {
using std::literals::string_view_literals::operator""sv;

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
Interval::Interval(size_t l, size_t u, bool bounded)
    : l(l), u(u), bounded(bounded) {
#ifndef NDEBUG
  if (bounded && l > u)
    throw std::invalid_argument("must have l <= u");
#endif
}

bool Interval::operator==(const Interval &other) const {
  if (bounded != other.bounded)
    return false;
  if (bounded)
    return l == other.l && u == other.u;
  else
    return l == other.l;
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
  if (!bounded)
    return n >= l;
  else
    return n >= l && n <= u;
}

bool Interval::is_bounded() const { return bounded; }

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

fv_set Term::fvi(size_t num_bound_vars) const {
  auto visitor = [num_bound_vars](auto &&arg) -> fv_set {
    using T = std::decay_t<decltype(arg)>;

    if constexpr (std::is_same_v<T, var_t>) {
      if (arg.idx >= num_bound_vars) {
        return fv_set({arg.idx - num_bound_vars});
      } else {
        return {};
      }
    } else if constexpr (any_type_equal_v<T, plus_t, minus_t, mult_t, div_t,
                                          mod_t>) {
      auto fvs1 = arg.l->fvi(num_bound_vars), fvs2 = arg.r->fvi(num_bound_vars);
      fvs1.insert(fvs2.begin(), fvs2.end());
      return fvs1;
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
Formula::Formula(const Formula &formula) : val(copy_val(formula.val)) {}
Formula::Formula(Formula &&formula) noexcept : val(std::move(formula.val)) {}
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
      return (arg1.pred_name == arg2.pred_name) &&
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
    return Formula::Pred(std::move(pred_name), std::move(trm_list));
  } else if (fo_ty == "Eq"sv) {
    return Formula::Eq(Term::from_json(json_formula.at(1)),
                       Term::from_json(json_formula.at(2)));
  } else if (fo_ty == "Less"sv) {
    return Formula::Less(Term::from_json(json_formula.at(1)),
                         Term::from_json(json_formula.at(2)));
  } else if (fo_ty == "LessEq"sv) {
    return Formula::LessEq(Term::from_json(json_formula.at(1)),
                           Term::from_json(json_formula.at(2)));
  } else if (fo_ty == "Neg"sv) {
    return Formula::Neg(Formula::from_json(json_formula.at(1)));
  } else if (fo_ty == "Or"sv) {
    return Formula::Or(Formula::from_json(json_formula.at(1)),
                       Formula::from_json(json_formula.at(2)));
  } else if (fo_ty == "And"sv) {
    return Formula::And(Formula::from_json(json_formula.at(1)),
                        Formula::from_json(json_formula.at(2)));
  } else if (fo_ty == "Exists"sv) {
    return Formula::Exists(Formula::from_json(json_formula.at(1)));
  } else if (fo_ty == "Prev"sv) {
    return Formula::Prev(Interval::from_json(json_formula.at(1)),
                         Formula::from_json(json_formula.at(2)));
  } else if (fo_ty == "Next"sv) {
    return Formula::Next(Interval::from_json(json_formula.at(1)),
                         Formula::from_json(json_formula.at(2)));
  } else if (fo_ty == "Since"sv) {
    return Formula::Since(Interval::from_json(json_formula.at(2)),
                          Formula::from_json(json_formula.at(1)),
                          Formula::from_json(json_formula.at(3)));
  } else if (fo_ty == "Until"sv) {
    return Formula::Until(Interval::from_json(json_formula.at(2)),
                          Formula::from_json(json_formula.at(1)),
                          Formula::from_json(json_formula.at(3)));
  } else {
    throw std::runtime_error("formula type not supported");
  }
}

Formula::Formula(val_type &&val) noexcept : val(std::move(val)) {}

Formula::Formula(const val_type &val) : Formula(copy_val(val)) {}

Formula Formula::Pred(name pred_name, vector<Term> pred_args) {
  return Formula(pred_t{std::move(pred_name), std::move(pred_args)});
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
      fvl.insert(fvr.begin(), fvr.end());
      return fvl;
    } else if constexpr (any_type_equal_v<T, prev_t, next_t, neg_t>) {
      return arg.phi->fvi(num_bound_vars);
    } else if constexpr (any_type_equal_v<T, and_t, or_t, since_t, until_t>) {
      auto fvl = arg.phil->fvi(num_bound_vars),
           fvr = arg.phir->fvi(num_bound_vars);
      fvl.insert(fvr.begin(), fvr.end());
      return fvl;
    } else if constexpr (is_same_v<T, exists_t>) {
      return arg.phi->fvi(num_bound_vars + 1);
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
      if (!arg.inter.is_bounded())
        return false;
      const auto &phil = *arg.phil, &phir = *arg.phir;
      if (is_subset(phil.fvs(), phir.fvs()) && phil.is_safe_formula())
        return true;
      if (!arg.phir->is_safe_formula())
        return false;
      if (const auto *neg_ptr = phil.inner_if_neg()) {
        return neg_ptr->is_safe_formula();
      }
      return false;
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}

/*
bool Formula::is_future_bounded() const {
  auto visitor = [](auto &&arg) -> bool {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, pred_t, eq_t, less_t, less_eq_t>) {
      return true;
    } else if constexpr (any_type_equal_v<T, neg_t, exists_t, prev_t, next_t>) {
      return arg.phi->is_future_bounded();
    } else if constexpr (any_type_equal_v<T, or_t, and_t, since_t>) {
      return arg.phil->is_future_bounded() && arg.phir->is_future_bounded();
    } else if constexpr (any_type_equal_v<T, until_t>) {
      return arg.phil->is_future_bounded() && arg.phir->is_future_bounded() &&
             arg.inter.is_bounded();
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}*/

/*bool Formula::is_monitorable() const {
  return is_safe_formula() && is_future_bounded();
}*/
}// namespace fo