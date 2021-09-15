#ifndef FORMULA_H
#define FORMULA_H

#include <absl/container/flat_hash_set.h>
#include <boost/variant2/variant.hpp>
#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace mfo::detail {
class MState;
}

namespace fo {
namespace detail {
  // Terms and formulas
  namespace var2 = boost::variant2;
  using std::vector;
  using var2::holds_alternative;
  using var2::variant;

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
    [[nodiscard]] bool is_const() const;
    [[nodiscard]] bool is_var() const;
    [[nodiscard]] size_t get_var() const;
    [[nodiscard]] const event_data &get_const() const;
    [[nodiscard]] fv_set comp_fv() const;

  private:
    [[nodiscard]] fv_set fvi(size_t nesting_depth) const;
    variant<Const, Var> term_val;
  };

  class Interval {
  public:
    Interval(size_t l, size_t u, bool bounded = true);
    [[nodiscard]] bool contains(size_t n) const;
    [[nodiscard]] bool is_bounded() const;

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
    friend class ::mfo::detail::MState;

  public:
    using val_type = variant<Pred, Less, LessEq, Eq, Or, And, Exists, Prev,
                             Next, Since, Until, Neg>;
    Formula(const Formula &formula);
    template<typename T>
    Formula(T &&arg) : val(std::forward<decltype(arg)>(arg)) {}
    explicit Formula(val_type &&val);
    explicit Formula(const val_type &val);
    [[nodiscard]] fv_set comp_fv() const;
    [[nodiscard]] size_t degree() const;
    [[nodiscard]] bool is_constraint() const;
    [[nodiscard]] bool is_safe_assignment(const fv_set &vars) const;
    [[nodiscard]] bool is_future_bounded() const { return false; }
    [[nodiscard]] bool is_safe_formula() const {
      // TODO: create implementation
      return false;
    }
    [[nodiscard]] bool is_monitorable() const;

  private:
    [[nodiscard]] static val_type copy_val(const val_type &val);
    [[nodiscard]] fv_set fvi(size_t nesting_depth) const;
    static inline constexpr auto uniq = [](auto &&arg) -> decltype(auto) {
      return std::make_unique<Formula>(std::forward<decltype(arg)>(arg));
    };
    val_type val;
    fv_set m_fvs;
    bool m_safe_formula;
    bool m_future_bounded;
    bool m_safe_assignment;
    bool m_constraint;
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