#ifndef FORMULA_H
#define FORMULA_H

#include <absl/container/flat_hash_set.h>
#include <boost/hana.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace fo {
using name = std::string;
using event_data = std::size_t;

using std::unique_ptr;
using std::variant;
using std::vector;

template <typename T> using ptr_type = unique_ptr<T>;

struct Const {
  event_data data;
};
struct Var {
  size_t idx;
};

using fv_set = absl::flat_hash_set<size_t>;

struct Term {
  variant<Const, Var> term_val;
  bool is_const() const { return std::holds_alternative<Const>(term_val); }
  bool is_var() const { return std::holds_alternative<Var>(term_val); }
  fv_set fvi(size_t nesting_depth) const {
    size_t var_idx;
    if (this->is_var() &&
        nesting_depth <= (var_idx = std::get<Var>(this->term_val).idx)) {
      return fv_set({var_idx - nesting_depth});
    } else {
      return fv_set();
    }
  }
};

// Use of the Curiously recurring template pattern for static polymorphism
struct Interval {};

struct Formula;
struct Pred {
  name pred_name;
  vector<Term> pred_args;
};

struct Eq {
  Term l, r;
};

struct Less {
  Term l, r;
};

struct LessEq {
  Term l, r;
};

struct Neg {
  ptr_type<Formula> phi;
};

struct Or {
  ptr_type<Formula> phil, phir;
};

struct And {
  ptr_type<Formula> phil, phir;
};

struct Exists {
  ptr_type<Formula> phi;
};

struct Prev {
  Interval inter;
  ptr_type<Formula> phi;
};

struct Next {
  Interval inter;
  ptr_type<Formula> phi;
};

struct Since {
  Interval inter;
  ptr_type<Formula> phil, phir;
};

struct Until {
  Interval inter;
  ptr_type<Formula> phil, phir;
};

/*
  Necessary plumbing for pattern matching
*/
namespace hana = boost::hana;
template <typename> inline constexpr bool always_false_v = false;
template <typename T, typename... Os> constexpr auto any_type_equal() {
  auto r = hana::any_of(hana::tuple_t<Os...>,
                        [](auto t) { return t == hana::type_c<T>; });
  return r;
}
template <typename T, typename... Os>
inline constexpr bool any_type_equal_v =
    decltype(any_type_equal<T, Os...>())::value;

struct Formula {
  variant<Pred, Less, LessEq, Eq, Or, And, Exists, Prev, Next, Since, Until>
      val;

  fv_set fvi(size_t nesting_depth) const {
    auto &&visitor = [nesting_depth](auto &&arg) {
      using T = std::decay_t<decltype(arg)>;
      if constexpr (std::is_same_v<T, Pred>) {
        fv_set fv_children;
        for (const auto &t : arg.pred_args) {
          fv_set child_set = t.fvi(nesting_depth);
          fv_children.insert(child_set.begin(), child_set.end());
        }
        return fv_children;
      } else if constexpr (any_type_equal_v<T, Less, LessEq, Eq>) {
        fv_set fvl = arg.l.fvi(nesting_depth), fvr = arg.r.fvi(nesting_depth);
        fvl.insert(fvr.begin(), fvr.end());
        return fvl;
      } else if constexpr (any_type_equal_v<T, Prev, Next>) {
        return arg.phi->fvi(nesting_depth);
      } else if constexpr (any_type_equal_v<T, And, Or, Since, Until>) {
        fv_set fvl = arg.phil->fvi(nesting_depth),
               fvr = arg.phir->fvi(nesting_depth);
        fvl.insert(fvr.begin(), fvr.end());
        return fvl;
      } else if constexpr (std::is_same_v<T, Exists>) {
        return arg.phi->fvi(nesting_depth + 1);
      } else {
        static_assert(always_false_v<T>, "non exhaustive!");
      }
    };
    return std::visit(visitor, this->val);
  }

  fv_set comp_fv() const { return fvi(0); }

  bool is_safe() const {
    auto &&visitor = [](auto &&arg) {
      if constexpr (true) {
        return false;
      } else {
        return true;
      }
    };
    return std::visit(visitor, this->val);
  }
};

} // namespace fo
#endif