#ifndef FORMULA_H
#define FORMULA_H

#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <boost/hana.hpp>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace fo {
namespace detail {
  // Utilities
  namespace hana = boost::hana;
  template<typename>
  inline constexpr bool always_false_v = false;
  template<typename T, typename... Os>
  inline constexpr auto any_type_equal() {
    auto r = hana::any_of(hana::tuple_t<Os...>,
                          [](auto t) { return t == hana::type_c<T>; });
    return r;
  }
  template<typename T, typename... Os>
  inline constexpr bool any_type_equal_v =
          decltype(any_type_equal<T, Os...>())::value;

  template<typename S>
  bool is_subset(const S &set1, const S &set2) {
    for (const auto &elem : set1) {
      if (!set2.contains(elem))
        return false;
    }
    return true;
  }

  // Terms and formulas
  using std::variant;
  using std::vector;

  using name       = std::string;
  using event_data = std::size_t;
  using fv_set     = absl::flat_hash_set<size_t>;

  template<typename T>
  using ptr_type = std::unique_ptr<T>;

  struct Const {
    event_data data;
  };

  struct Var {
    size_t idx;
  };

  struct Formula;

  struct Term {
    friend Formula;

  public:
    variant<Const, Var> term_val;
    [[nodiscard]] bool is_const() const;
    [[nodiscard]] bool is_var() const;
    [[nodiscard]] size_t get_var() const;
    [[nodiscard]] fv_set comp_fv() const;

  private:
    [[nodiscard]] fv_set fvi(size_t nesting_depth) const;
  };

  class Interval {
  public:
    Interval(size_t l, size_t u, bool bounded = true);

  private:
    size_t l, u;
    bool bounded;
  };

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

  struct Formula {
  public:
    using val_type = variant<Pred, Less, LessEq, Eq, Or, And, Exists, Prev,
                             Next, Since, Until, Neg>;
    Formula(const Formula &formula);
    explicit Formula(val_type &&val);
    explicit Formula(const val_type &val);
    [[nodiscard]] fv_set comp_fv() const;
    [[nodiscard]] size_t degree() const;
    [[nodiscard]] bool is_constraint() const;
    [[nodiscard]] bool is_safe_assignment(const fv_set &vars) const;
    [[nodiscard]] bool is_safe_formula() const;

  private:
    [[nodiscard]] static val_type copy_val(const val_type &val);
    [[nodiscard]] fv_set fvi(size_t nesting_depth) const;
    val_type val;
  };
}// namespace detail

using detail::And;
using detail::Const;
using detail::Eq;
using detail::event_data;
using detail::Exists;
using detail::Formula;
using detail::fv_set;
using detail::Interval;
using detail::Less;
using detail::LessEq;
using detail::name;
using detail::Neg;
using detail::Next;
using detail::Or;
using detail::Pred;
using detail::Prev;
using detail::Since;
using detail::Term;
using detail::Until;
using detail::Var;

}// namespace fo
#endif