#ifndef MFORMULA_H
#define MFORMULA_H

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <boost/variant2/variant.hpp>
#include <formula.h>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <table.h>
#include <type_traits>
#include <util.h>
#include <variant>
#include <iterator>
#include <algorithm>
#include <vector>

namespace monitor {
namespace detail {
  namespace var2 = boost::variant2;
  using absl::flat_hash_map;
  using absl::flat_hash_set;
  using std::optional;
  using std::pair;
  using std::string_view;
  using std::vector;
  using var2::variant;

  using fo::event_data;
  using fo::Formula;
  using fo::fv_set;
  using fo::Interval;
  using fo::name;
  using fo::Term;
  using fo::detail::ptr_type;

  using table_type = table<event_data>;
  using satisfactions_t = vector<pair<size_t, table_type>>;
  class Monitor;

  // TODO: check if this type is correct
  // TODO: optimize translating predicate names to integers
  using db_data = vector<flat_hash_set<vector<event_data>>>;
  using database = flat_hash_map<name, db_data>;

  // TODO: implement this later together with meval
  struct MBuf2 {};

  // TODO: implement this later together with meval
  struct MSaux {};

  // TODO: implement this later together with meval
  struct MUaux {};

  class MState;

  struct MRel {
    table_type tab;
    vector<table_type> eval(const database &db, size_t n_tps, size_t ts) const;
  };

  struct MPred {
    name pred_name;
    vector<Term> pred_args;
    size_t n_fvs;
    vector<table_type> eval(const database &db, size_t n_tps, size_t ts) const;
    optional<vector<event_data>> match(const vector<event_data> &event_args) const;
  };

  struct MAnd {
    bool is_positive;
    MBuf2 buf;
    ptr_type<MState> l_state, r_state;
  };

  // TODO: implement this later together with meval
  struct MAndAssign {};

  // TODO: implement this later together with meval
  struct MAndRel {};


  struct MOr {
    ptr_type<MState> l_state, r_state;
    MBuf2 buf;
  };

  struct MNeg {
    ptr_type<MState> state;
    vector<table_type> eval(const database &db, size_t n_tps, size_t ts) const;
  };

  struct MExists {
    size_t bound_var;
    ptr_type<MState> state;
  };

  struct MPrev {
    Interval inter;
    bool is_first;
    vector<table_type> past_data;
    vector<size_t> past_ts;
    ptr_type<MState> state;
  };

  struct MNext {
    Interval inter;
    bool is_first;
    vector<size_t> past_ts;
    ptr_type<MState> state;
  };

  struct MSince {};

  struct MUntil {};


  class MState {
    friend class Monitor;
    friend struct MNeg;

  private:
    vector<table_type> eval(const database &db, size_t n_tps, size_t ts);
    using val_type = variant<MRel, MPred, MOr, MExists, MPrev, MNext, MNeg,
                             MAndRel, MAndAssign, MAnd>;
    explicit MState(val_type &&state);
    static inline constexpr auto uniq = [](auto &&arg) -> decltype(auto) {
      return std::make_unique<MState>(std::forward<decltype(arg)>(arg));
    };
    static pair<val_type, table_layout>
    make_eq_state(const fo::Formula::eq_t &arg);

    static pair<val_type, table_layout>
    make_and_state(const fo::Formula::and_t &arg, size_t n_bound_vars);

    static pair<val_type, table_layout>
    make_formula_state(const Formula &formula, size_t n_bound_vars);
    val_type state;
  };

  class Monitor {
  public:
    explicit Monitor(const Formula &formula);
    satisfactions_t step(const database &db, size_t ts);

  private:
    optional<MState> state;
    table_layout res_layout;
    size_t curr_tp;
  };

}// namespace detail

using detail::Monitor;
using detail::table_type;

}// namespace monitor

#endif