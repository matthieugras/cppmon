#ifndef FORMULA_H
#define FORMULA_H

#define JSON_USE_IMPLICIT_CONVERSIONS 0
#include <absl/container/flat_hash_set.h>
#include <boost/variant2/variant.hpp>
#include <cstddef>
#include <memory>
#include <nlohmann/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>
#include <optional>
#include <boost/operators.hpp>

namespace mfo::detail {
class MState;
}

namespace fo {
namespace detail {
  // Terms and formulas
  namespace var2 = boost::variant2;

  using nlohmann::json;
  using boost::equality_comparable;
  using std::string;
  using std::string_view;
  using std::vector;
  using std::optional;
  using var2::holds_alternative;
  using var2::variant;
  using std::literals::string_view_literals::operator""sv;

  using name = std::string;
  using fv_set = absl::flat_hash_set<size_t>;

  size_t nat_from_json(const json &json_formula);
  optional<size_t> enat_from_json(const json &json_formula);

  class event_t: equality_comparable<event_t> {
    friend struct Term;

  public:
    bool operator==(const event_t &other) const;
    static event_t Int(int i);
    static event_t Float(double d);
    static event_t String(string s);
    static event_t from_json(const json &json_formula);

  private:
    using val_type = variant<int, double, string>;
    explicit event_t(val_type &&val) noexcept;
    val_type val;
  };

  template<typename T>
  using ptr_type = std::unique_ptr<T>;

  struct Formula;

  struct Term: equality_comparable<Term> {
    friend Formula;

  public:
    Term(const Term &t);
    Term(Term &&t) noexcept;
    bool operator==(const Term &other) const;
    static Term Var(size_t idx);
    static Term Const(event_t c);
    static Term Plus(Term l, Term r);
    static Term Minus(Term l, Term r);
    static Term UMinus(Term t);
    static Term Mult(Term l, Term r);
    static Term Div(Term l, Term r);
    static Term Mod(Term l, Term r);
    static Term F2i(Term t);
    static Term I2f(Term t);
    [[nodiscard]] bool is_const() const;
    [[nodiscard]] bool is_var() const;
    [[nodiscard]] size_t get_var() const;
    [[nodiscard]] fv_set fvs() const;

  private:
    struct var_t {
      size_t idx;
    };
    struct plus_t {
      ptr_type<Term> l, r;
    };
    struct minus_t {
      ptr_type<Term> l, r;
    };
    struct uminus_t {
      ptr_type<Term> t;
    };
    struct mult_t {
      ptr_type<Term> l, r;
    };
    struct div_t {
      ptr_type<Term> l, r;
    };
    struct mod_t {
      ptr_type<Term> l, r;
    };
    struct f2i_t {
      ptr_type<Term> t;
    };
    struct i2f_t {
      ptr_type<Term> t;
    };
    using val_type = variant<event_t, var_t, plus_t, minus_t, uminus_t, mult_t, div_t,
                             mod_t, f2i_t, i2f_t>;
    explicit Term(val_type &&val) noexcept;
    explicit Term(const val_type &val);
    template<typename F>
    static inline std::unique_ptr<Term> uniq(F &&arg) {
      return std::unique_ptr<Term>(new Term(std::forward<F>(arg)));
    }
    [[nodiscard]] static val_type copy_val(const val_type &val);
    [[nodiscard]] fv_set fvi(size_t num_bound_vars) const;
    static Term from_json(const json &json_formula);
    val_type val;
  };

  class Interval: equality_comparable<Interval> {
    friend Formula;

  public:
    Interval(size_t l, size_t u, bool bounded = true);
    bool operator==(const Interval &other) const;
    [[nodiscard]] bool contains(size_t n) const;
    [[nodiscard]] bool is_bounded() const;

  private:
    static Interval from_json(const json &json_formula);
    size_t l, u;
    bool bounded;
  };

  struct Formula: equality_comparable<Formula> {
    friend class ::mfo::detail::MState;

  public:
    Formula(const Formula &formula);
    Formula(Formula &&formula) noexcept;
    bool operator==(const Formula &other) const;
    explicit Formula(std::string_view json_formula);
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
    static Formula from_json(const json &json_formula);
    explicit Formula(val_type &&val) noexcept;
    explicit Formula(const val_type &val);
    [[nodiscard]] static val_type copy_val(const val_type &val);
    [[nodiscard]] fv_set fvi(size_t num_bound_vars) const;
    template<typename F>
    static inline std::unique_ptr<Formula> uniq(F &&arg) {
      return std::unique_ptr<Formula>(new Formula(std::forward<F>(arg)));
    }
    val_type val;
  };
}// namespace detail

using detail::event_t;
using detail::Formula;
using detail::fv_set;
using detail::Interval;
using detail::name;
using detail::Term;

}// namespace fo
#endif