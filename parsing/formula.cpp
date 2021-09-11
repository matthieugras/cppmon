#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <boost/hana.hpp>
#include <formula.h>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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
      static_assert(always_false_v<T>, "non exhaustive!");
    }
  };
  return std::visit(visitor, this->val);
}

[[nodiscard]] fv_set Formula::comp_fv() const { return fvi(0); }

[[nodiscard]] bool Formula::is_safe_assignment() const { return false; }

[[nodiscard]] bool Formula::is_safe_formula() const {
  using std::get;
  using std::is_same_v;
  auto visitor = [](auto &&arg) {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (is_same_v<T, Eq>) {
      return (arg.l.is_const() && (arg.r.is_const() || arg.r.is_var())) ||
             (arg.l.is_var() && arg.r.is_const());
    } else if constexpr (any_type_equal_v<T, Less, LessEq>) {
      return false;
    } else if constexpr (is_same_v<T, Neg>) {
      if (std::holds_alternative<Eq>(arg.phi->val)) {
        const auto &t1 = get<Eq>(arg.phi->val).l, &t2 = get<Eq>(arg.phi->val).r;
        if (t1.is_var() && t2.is_var()) return t1.get_var() == t2.get_var();
      }
      return arg.phi->comp_fv().empty() && arg.phi->is_safe_formula();
    } else if constexpr (is_same_v<T, Pred>) {
      return std::all_of(arg.pred_args.begin(), arg.pred_args.end(),
                         [](auto &&t) { return t.is_var() || t.is_const(); });
    } else if constexpr (is_same_v<T, Or>) {
      auto fvl = arg.phil->comp_fv(), fvr = arg.phir->comp_fv();
      if (fvl != fvr) return false;
      return arg.phil->is_safe_formula() && arg.phir->is_safe_formula();
    } else if constexpr (is_same_v<T, And>) {
      return false;
    } else if constexpr (any_type_equal_v<T, Exists, Prev, Next>) {
      return arg.phi->is_safe_formula();
    } else if constexpr (any_type_equal_v<T, Since, Until>) {
      if (is_subset(arg.phil->comp_fv(), arg.phir->comp_fv()) &&
          arg.phil->is_safe_formula())
        return true;
      if (!arg.phir->is_safe_formula()) return false;
      if (std::holds_alternative<Neg>(arg.phil->val) &&
          get<Neg>(arg.phil->val).phi->is_safe_formula())
        return true;
      else
        return false;
    } else {
      return false;
    }
  };
  return std::visit(visitor, this->val);
}
}// namespace fo::detail