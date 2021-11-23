#ifndef FORMULA_H
#define FORMULA_H

#include <absl/container/flat_hash_set.h>
#include <absl/hash/hash.h>
#include <absl/random/random.h>
#include <boost/operators.hpp>
#include <boost/variant2/variant.hpp>
#include <cstddef>
#include <event_data.h>
#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

namespace monitor::detail {
class MState;
struct MPred;
}// namespace monitor::detail

class dbgen;

namespace fo {
namespace {
  namespace var2 = boost::variant2;
  using boost::equality_comparable;
  using common::event_data;
  using nlohmann::json;
  using std::optional;
  using std::string;
  using std::string_view;
  using std::vector;
  using var2::variant;
}// namespace


enum class agg_type
{
  CNT,
  MIN,
  MAX,
  SUM,
  AVG,
  MED
};

agg_type agg_type_from_json(const json &json_formula);


using name = std::string;
using fv_set = absl::flat_hash_set<size_t>;

fv_set fvi_single_var(size_t var, size_t num_bound_vars);

size_t nat_from_json(const json &json_formula);
optional<size_t> enat_from_json(const json &json_formula);

template<typename T>
using ptr_type = std::unique_ptr<T>;

struct Formula;

struct Term : equality_comparable<Term> {
  friend Formula;
  friend ::monitor::detail::MPred;
  friend ::dbgen;
  friend class ::monitor::detail::MState;

public:
  Term(const Term &t);
  Term(Term &&t) noexcept = default;
  Term &operator=(Term &&other) = default;
  bool operator==(const Term &other) const;
  static Term Var(size_t idx);
  static Term Const(event_data c);
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
  [[nodiscard]] const size_t *get_if_var() const;
  [[nodiscard]] const event_data *get_if_const() const;
  [[nodiscard]] fv_set fvs() const;
  [[nodiscard]] event_data eval(const vector<size_t> &var_2_idx,
                                const vector<event_data> &tuple) const;

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
  using val_type = variant<event_data, var_t, plus_t, minus_t, uminus_t, mult_t,
                           div_t, mod_t, f2i_t, i2f_t>;
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

class Interval : equality_comparable<Interval> {
  friend Formula;

public:
  Interval(size_t lower_bound, size_t upper_bound, bool bounded = true);
  bool operator==(const Interval &other) const;
  [[nodiscard]] bool leq_upper(size_t n) const;
  [[nodiscard]] bool gt_upper(size_t n) const;
  [[nodiscard]] bool geq_lower(size_t n) const;
  [[nodiscard]] bool lt_lower(size_t n) const;
  [[nodiscard]] bool contains(size_t n) const;
  [[nodiscard]] bool is_bounded() const;
  [[nodiscard]] size_t get_lower() const;
  template<typename Rnd>
  size_t random_sample(Rnd &bitgen) const {
    if (!bounded_)
      throw std::runtime_error("interval is not bounded");
    return absl::Uniform<size_t>(bitgen, l_, u_);
  }

private:
  static Interval from_json(const json &json_formula);
  size_t l_, u_;
  bool bounded_;
};

struct Formula : equality_comparable<Formula> {
  friend class ::monitor::detail::MState;
  friend class ::dbgen;

public:
  Formula(const Formula &formula);
  Formula(Formula &&formula) noexcept;
  size_t unique_id() const;
  bool operator==(const Formula &other) const;
  explicit Formula(std::string_view json_formula);
  static Formula Pred(name pred_name, vector<Term> pred_args, bool is_builtin);
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
  static Formula Agg(agg_type ty, size_t res_var, size_t num_bound_vars,
                     event_data default_value, Term agg_term, Formula phi);
  static Formula Let(std::string pred_name, Formula phi, Formula psi);
  [[nodiscard]] Formula *inner_if_neg() const;
  [[nodiscard]] fv_set fvs() const;
  [[nodiscard]] size_t degree() const;
  [[nodiscard]] bool is_constraint() const;
  [[nodiscard]] bool is_safe_assignment(const fv_set &vars) const;
  [[nodiscard]] bool is_safe_formula() const;
  [[nodiscard]] bool is_always_true() const;


private:
  static size_t formula_id_counter;

  struct pred_t {
    name pred_name;
    vector<Term> pred_args;
    bool is_builtin;
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

  struct agg_t {
    agg_type ty;
    size_t res_var;
    size_t num_bound_vars;
    event_data default_value;
    Term agg_term;
    ptr_type<Formula> phi;
  };

  struct let_t {
    std::string pred_name;
    ptr_type<Formula> phi, psi;
  };

  using val_type =
    variant<pred_t, less_t, less_eq_t, eq_t, or_t, and_t, exists_t, prev_t,
            next_t, since_t, until_t, neg_t, agg_t, let_t>;
  static Formula from_json(const json &json_formula);
  explicit Formula(val_type &&val) noexcept;
  explicit Formula(const val_type &val);
  [[nodiscard]] static val_type copy_val(const val_type &val);
  [[nodiscard]] fv_set fvi(size_t num_bound_vars) const;
  template<typename F>
  static inline std::unique_ptr<Formula> uniq(F &&arg) {
    return std::unique_ptr<Formula>(new Formula(std::forward<F>(arg)));
  }
  size_t formula_id_;
  val_type val;
};

}// namespace fo
#endif