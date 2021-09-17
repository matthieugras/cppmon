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

  using name = std::string;
  using event_data = std::size_t;
  using fv_set = absl::flat_hash_set<size_t>;

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
    [[nodiscard]] fv_set fvs() const;
    variant<Const, Var> term_val;

  private:
    [[nodiscard]] fv_set fvi(size_t num_bound_vars) const;
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

  struct Formula {
    friend class ::mfo::detail::MState;

  public:
    Formula(const Formula &formula);
    Formula(Formula &&formula) noexcept;
    static Formula Pred(name pred_name, vector<Term> pred_args);
    static Formula Eq(Term l, Term r);
    static Formula Less(Term l, Term r);
    static Formula LessEq(Term l, Term r);
    static Formula Neg(Formula phi);
    static Formula Or(Formula phil, Formula phir);
    static Formula And(Formula phil, Formula phir);
    static Formula Exists(Formula phi);
    static Formula Prev(Interval inter, Formula phi);
    static Formula Next(Interval inter, Formula phi);
    static Formula Since(Interval inter, Formula phil, Formula phir);
    static Formula Until(Interval inter, Formula phil, Formula phir);
    [[nodiscard]] fv_set fvs() const;
    [[nodiscard]] size_t degree() const;
    [[nodiscard]] bool is_constraint() const;
    [[nodiscard]] bool is_safe_assignment(const fv_set &vars) const;
    [[nodiscard]] bool is_future_bounded() const;
    [[nodiscard]] bool is_safe_formula() const;
    [[nodiscard]] bool is_monitorable() const;

  private:
    struct pred_t {
      name pred_name;
      vector<Term> pred_args;
    };

    struct eq_t {
      Term l, r;
    };

    struct less_t {
      Term l, r;
    };

    struct less_eq_t {
      Term l, r;
    };

    struct neg_t {
      ptr_type<Formula> phi;
    };

    struct or_t {
      ptr_type<Formula> phil, phir;
    };

    struct and_t {
      ptr_type<Formula> phil, phir;
    };

    struct exists_t {
      ptr_type<Formula> phi;
    };

    struct prev_t {
      Interval inter;
      ptr_type<Formula> phi;
    };

    struct next_t {
      Interval inter;
      ptr_type<Formula> phi;
    };

    struct since_t {
      Interval inter;
      ptr_type<Formula> phil, phir;
    };

    struct until_t {
      Interval inter;
      ptr_type<Formula> phil, phir;
    };
    using val_type = variant<pred_t, less_t, less_eq_t, eq_t, or_t, and_t,
                             exists_t, prev_t, next_t, since_t, until_t, neg_t>;
    explicit Formula(val_type &&val);
    explicit Formula(const val_type &val);
    [[nodiscard]] static val_type copy_val(const val_type &val);
    [[nodiscard]] fv_set fvi(size_t num_bound_vars) const;
    // Like make_unique, but we can't use it because of private constructors
    template<typename F>
    static inline std::unique_ptr<Formula> uniq(F &&arg) {
      return std::unique_ptr<Formula>(new Formula(std::forward<F>(arg)));
    }
    val_type val;
  };
}// namespace detail

using detail::Const;
using detail::event_data;
using detail::Formula;
using detail::fv_set;
using detail::Interval;
using detail::name;
using detail::Term;
using detail::Var;

}// namespace fo
#endif