#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <formula.h>
#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

namespace fo::detail {

// Term member functions
[[nodiscard]] bool Term::is_const() const {
  return std::holds_alternative<Const>(term_val);
}
[[nodiscard]] bool Term::is_var() const {
  return std::holds_alternative<Var>(term_val);
}
[[nodiscard]] size_t Term::get_var() const {
  return std::get<Var>(this->term_val).idx;
}
[[nodiscard]] fv_set Term::fvi(size_t nesting_depth) const {
  size_t var_idx;
  if (this->is_var() &&
      nesting_depth <= (var_idx = std::get<Var>(this->term_val).idx)) {
    return fv_set({var_idx - nesting_depth});
  } else {
    return {};
  }
}
[[nodiscard]] fv_set Term::comp_fv() const { return fvi(0); }


// Formula member functions
Formula::Formula(const Formula &formula) : val(copy_val(formula.val)) {}

Formula::Formula(val_type &&val) : val(std::move(val)) {}

Formula::Formula(const val_type &val) : Formula(copy_val(val)) {}

[[nodiscard]] Formula::val_type Formula::copy_val(const val_type &val) {
  auto visitor = [](auto &&arg) -> Formula::val_type {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, Pred, Eq, Less, LessEq>) {
      return arg;
    } else if constexpr (any_type_equal_v<T, Neg, Exists>) {
      return T{std::make_unique<Formula>(copy_val(arg.phi->val))};
    } else if constexpr (any_type_equal_v<T, Or, And>) {
      return T{std::make_unique<Formula>(copy_val(arg.phil->val)),
               std::make_unique<Formula>(copy_val(arg.phir->val))};
    } else if constexpr (any_type_equal_v<T, Prev, Next>) {
      return T{arg.inter, std::make_unique<Formula>(copy_val(arg.phi->val))};
    } else if constexpr (any_type_equal_v<T, Since, Until>) {
      return T{arg.inter, std::make_unique<Formula>(copy_val(arg.phil->val)),
               std::make_unique<Formula>(copy_val(arg.phir->val))};
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return std::visit(visitor, val);
}

[[nodiscard]] fv_set Formula::fvi(size_t nesting_depth) const {
  auto &&visitor = [nesting_depth](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    using std::is_same_v;
    if constexpr (is_same_v<T, Pred>) {
      fv_set fv_children;
      for (const auto &t : arg.pred_args) {
        auto child_set = t.fvi(nesting_depth);
        fv_children.insert(child_set.begin(), child_set.end());
      }
      return fv_children;
    } else if constexpr (any_type_equal_v<T, Less, LessEq, Eq>) {
      auto fvl = arg.l.fvi(nesting_depth), fvr = arg.r.fvi(nesting_depth);
      fvl.insert(fvr.begin(), fvr.end());
      return fvl;
    } else if constexpr (any_type_equal_v<T, Prev, Next, Neg>) {
      return arg.phi->fvi(nesting_depth);
    } else if constexpr (any_type_equal_v<T, And, Or, Since, Until>) {
      auto fvl = arg.phil->fvi(nesting_depth),
           fvr = arg.phir->fvi(nesting_depth);
      fvl.insert(fvr.begin(), fvr.end());
      return fvl;
    } else if constexpr (is_same_v<T, Exists>) {
      return arg.phi->fvi(nesting_depth + 1);
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return std::visit(visitor, this->val);
}

[[nodiscard]] fv_set Formula::comp_fv() const { return fvi(0); }

[[nodiscard]] bool Formula::is_constraint() const {
  auto visitor = [](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, Eq, Less, LessEq>) {
      return true;
    } else if constexpr (std::is_same_v<T, Neg>) {
      return arg.phi->is_constraint();
    } else {
      return false;
    }
  };
  return std::visit(visitor, this->val);
}

[[nodiscard]] bool Formula::is_safe_assignment(const fv_set &vars) const {
  using std::get;
  if (std::holds_alternative<Eq>(this->val)) {
    const auto &t1 = get<Eq>(this->val).l, &t2 = get<Eq>(this->val).r;
    if (t1.is_var() && t2.is_var()) {
      size_t var1 = t1.get_var(), var2 = t2.get_var();
      return vars.contains(var1) != vars.contains(var2);
    } else if (t1.is_var()) {
      return (!vars.contains(t1.get_var())) && is_subset(t2.comp_fv(), vars);
    } else if (t2.is_var()) {
      return (!vars.contains(t2.get_var()) && is_subset(t1.comp_fv(), vars));
    }
  }
  return false;
}

[[nodiscard]] bool Formula::is_safe_formula() const {
  using std::get;
  using std::is_same_v;
  auto visitor = [](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (is_same_v<T, Eq>) {
      const auto &[l, r] = arg;
      return (l.is_const() && (r.is_const() || r.is_var())) ||
             (l.is_var() && r.is_const());
    } else if constexpr (any_type_equal_v<T, Less, LessEq>) {
      return false;
    } else if constexpr (is_same_v<T, Neg>) {
      if (std::holds_alternative<Eq>(arg.phi->val)) {
        const auto &t1 = get<Eq>(arg.phi->val).l, &t2 = get<Eq>(arg.phi->val).r;
        if (t1.is_var() && t2.is_var())
          return t1.get_var() == t2.get_var();
      }
      return arg.phi->comp_fv().empty() && arg.phi->is_safe_formula();
    } else if constexpr (is_same_v<T, Pred>) {
      return std::all_of(arg.pred_args.begin(), arg.pred_args.end(),
                         [](auto &&t) { return t.is_var() || t.is_const(); });
    } else if constexpr (is_same_v<T, Or>) {
      const auto &[phil, phir] = arg;
      auto fvl = phil->comp_fv(), fvr = phir->comp_fv();
      if (fvl != fvr)
        return false;
      return phil->is_safe_formula() && phir->is_safe_formula();
    } else if constexpr (is_same_v<T, And>) {
      const auto &[phil, phir] = arg;
      if (!phil->is_safe_formula())
        return false;
      if (phir->is_safe_assignment(phil->comp_fv()) || phir->is_safe_formula())
        return true;
      if (!is_subset(phir->comp_fv(), phil->comp_fv()))
        return false;
      return phir->is_constraint() ||
             (std::holds_alternative<Neg>(phir->val) &&
              std::get<Neg>(phir->val).phi->is_safe_formula());
    } else if constexpr (any_type_equal_v<T, Exists, Prev, Next>) {
      return arg.phi->is_safe_formula();
    } else if constexpr (any_type_equal_v<T, Since, Until>) {
      const auto &phil = arg.phil, &phir = arg.phir;
      if (is_subset(phil->comp_fv(), phir->comp_fv()) &&
          phil->is_safe_formula())
        return true;
      if (!arg.phir->is_safe_formula())
        return false;
      return (std::holds_alternative<Neg>(phil->val) &&
              get<Neg>(phil->val).phi->is_safe_formula());
    } else {
      static_assert(always_false_v<T>, "not exhaustive");
    }
  };
  return std::visit(visitor, this->val);
}
}// namespace fo::detail