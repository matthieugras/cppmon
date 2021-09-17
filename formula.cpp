#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <formula.h>
#include <type_traits>
#include <util.h>
#include <utility>

namespace fo::detail {

// Interval member function
Interval::Interval(size_t l, size_t u, bool bounded)
    : l(l), u(u), bounded(bounded) {
#ifndef NDEBUG
  if (bounded && l > u)
    throw std::invalid_argument("l <= u");
#endif
}

bool Interval::contains(size_t n) const {
  if (!bounded)
    return n >= l;
  else
    return n >= l && n >= u;
}

bool Interval::is_bounded() const { return bounded; }

// Term member functions
bool Term::is_const() const { return holds_alternative<Const>(term_val); }

bool Term::is_var() const { return holds_alternative<Var>(term_val); }

size_t Term::get_var() const { return var2::get<Var>(term_val).idx; }

const event_data &Term::get_const() const {
  return var2::get<Const>(term_val).data;
}

fv_set Term::fvi(size_t num_bound_vars) const {
  if (const Var *ptr = var2::get_if<Var>(&term_val); ptr) {
    return fv_set({ptr->idx - num_bound_vars});
  } else {
    return {};
  }
}
fv_set Term::fvs() const { return fvi(0); }

// Formula member functions
Formula::Formula(const Formula &formula) : val(copy_val(formula.val)) {}
Formula::Formula(Formula &&formula) noexcept : val(std::move(formula.val)) {}

Formula::Formula(val_type &&val) : val(std::move(val)) {}

Formula::Formula(const val_type &val) : Formula(copy_val(val)) {}

Formula Formula::Pred(name pred_name, vector<Term> pred_args) {
  return Formula(Formula::pred_t{std::move(pred_name), std::move(pred_args)});
}
Formula Formula::Eq(Term l, Term r) {
  return Formula(Formula::eq_t{std::move(l), std::move(r)});
}
Formula Formula::Less(Term l, Term r) {
  return Formula(Formula::less_t{std::move(l), std::move(r)});
}
Formula Formula::LessEq(Term l, Term r) {
  return Formula(Formula::less_eq_t{std::move(l), std::move(r)});
}
Formula Formula::Neg(Formula phi) {
  return Formula(Formula::neg_t{uniq(std::move(phi))});
}
Formula Formula::Or(Formula phil, Formula phir) {
  return Formula(Formula::or_t{uniq(std::move(phil)), uniq(std::move(phir))});
}
Formula Formula::And(Formula phil, Formula phir) {
  return Formula(Formula::and_t{uniq(std::move(phil)), uniq(std::move(phir))});
}
Formula Formula::Exists(Formula phi) {
  return Formula(Formula::exists_t{uniq(std::move(phi))});
}
Formula Formula::Prev(Interval inter, Formula phi) {
  return Formula(Formula::prev_t{inter, uniq(std::move(phi))});
}
Formula Formula::Next(Interval inter, Formula phi) {
  return Formula(Formula::next_t{inter, uniq(std::move(phi))});
}
Formula Formula::Since(Interval inter, Formula phil, Formula phir) {
  return Formula(
    Formula::since_t{inter, uniq(std::move(phil)), uniq(std::move(phir))});
}
Formula Formula::Until(Interval inter, Formula phil, Formula phir) {
  return Formula(
    Formula::until_t{inter, uniq(std::move(phil)), uniq(std::move(phir))});
}

Formula::val_type Formula::copy_val(const val_type &val) {
  auto visitor = [](auto &&arg) -> Formula::val_type {
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
  fv_set this_fvs = fvs();
  if (this_fvs.empty())
    return 0;
  else
    return *std::max_element(this_fvs.begin(), this_fvs.end()) + 1;
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
    if (t1.is_var() && t2.is_var()) {
      size_t var1 = t1.get_var(), var2 = t2.get_var();
      return vars.contains(var1) != vars.contains(var2);
    } else if (t1.is_var()) {
      return (!vars.contains(t1.get_var())) && is_subset(t2.fvs(), vars);
    } else if (t2.is_var()) {
      return (!vars.contains(t2.get_var()) && is_subset(t1.fvs(), vars));
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
        const auto &t1 = ptr->l, &t2 = ptr->r;
        if (t1.is_var() && t2.is_var())
          return t1.get_var() == t2.get_var();
      }
      return arg.phi->fvs().empty() && arg.phi->is_monitorable();
    } else if constexpr (is_same_v<T, pred_t>) {
      return std::all_of(
        arg.pred_args.begin(), arg.pred_args.end(),
        [](const auto &t) { return t.is_var() || t.is_const(); });
    } else if constexpr (is_same_v<T, or_t>) {
      const auto &[phil, phir] = arg;
      auto fvl = phil->fvs(), fvr = phir->fvs();
      if (fvl != fvr)
        return false;
      return phil->is_monitorable() && phir->is_monitorable();
    } else if constexpr (is_same_v<T, and_t>) {
      const auto &[phil, phir] = arg;
      if (!phil->is_monitorable())
        return false;
      if (phir->is_safe_assignment(phil->fvs()) || phir->is_monitorable())
        return true;
      if (!is_subset(phir->fvs(), phil->fvs()))
        return false;
      const auto *neg_ptr = var2::get_if<neg_t>(&phir->val);
      return phir->is_constraint() ||
             (neg_ptr && neg_ptr->phi->is_monitorable());
    } else if constexpr (any_type_equal_v<T, exists_t, prev_t, next_t>) {
      return arg.phi->is_monitorable();
    } else if constexpr (any_type_equal_v<T, since_t, until_t>) {
      if (!arg.inter.is_bounded())
        return false;
      const auto &phil = arg.phil, &phir = arg.phir;
      if (is_subset(phil->fvs(), phir->fvs()) && phil->is_monitorable())
        return true;
      if (!arg.phir->is_monitorable())
        return false;
      const auto *neg_ptr = var2::get_if<neg_t>(&phil->val);
      return (neg_ptr && neg_ptr->phi->is_monitorable());
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return var2::visit(visitor, val);
}

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
}

bool Formula::is_monitorable() const {
  return is_safe_formula() && is_future_bounded();
}
}// namespace fo::detail