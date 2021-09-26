#ifndef MFORMULA_H
#define MFORMULA_H

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <boost/container/devector.hpp>
#include <boost/variant2/variant.hpp>
#include <event_data.h>
#include <formula.h>
#include <iterator>
#include <optional>
#include <stdexcept>
#include <string_view>
#include <table.h>
#include <traceparser.h>
#include <type_traits>
#include <util.h>
#include <variant>
#include <vector>

namespace monitor {
namespace detail {
  namespace var2 = boost::variant2;
  using absl::flat_hash_map;
  using absl::flat_hash_set;
  using boost::container::devector;
  using std::optional;
  using std::pair;
  using std::string_view;
  using std::vector;
  using var2::variant;

  using common::event_data;
  using fo::Formula;
  using fo::fv_set;
  using fo::Interval;
  using fo::name;
  using fo::ptr_type;
  using fo::Term;

  using event_table = table<event_data>;
  using satisfactions = vector<pair<size_t, event_table>>;
  using database = parse::database;
  class monitor;

  class BinaryBuffer {
  public:
    BinaryBuffer() : buf(), is_l(false){};

    template<typename F>
    vector<event_table> update_and_reduce(vector<event_table> &new_l,
                                          vector<event_table> &new_r, F &f) {
      vector<event_table> res;
      auto it1 = new_l.begin(), eit1 = new_l.end(), it2 = new_r.begin(),
           eit2 = new_r.end();
      if (!is_l) {
        std::swap(it1, it2);
        std::swap(eit1, eit2);
      }
      for (; !buf.empty() && it2 != eit2; buf.pop_front(), ++it2) {
        if (is_l)
          res.push_back(f(buf.front(), *it2));
        else
          res.push_back(f(*it2, buf.front()));
      }
      for (; it1 != eit1 && it2 != eit2; it1++, it2++) {
        if (is_l)
          res.push_back(f(*it1, *it2));
        else
          res.push_back(f(*it2, *it1));
      }
      if (it1 == eit1) {
        is_l = !is_l;
        std::swap(it1, it2);
        std::swap(eit1, eit2);
      }
      buf.insert(buf.end(), std::make_move_iterator(it1),
                 std::make_move_iterator(eit1));
      return res;
    }

  private:
    devector<event_table> buf;
    bool is_l;
  };

  // TODO: implement this later together with meval
  struct MSaux {};

  // TODO: implement this later together with meval
  struct MUaux {};

  class MState;

  struct MRel {
    event_table tab;
    vector<event_table> eval(const database &db, size_t ts) const;
  };

  struct MPred {
    name pred_name;
    vector<Term> pred_args;
    size_t n_fvs;
    vector<event_table> eval(const database &db, size_t ts) const;
    optional<vector<event_data>>
    // variables
    match(const vector<event_data> &event_args) const;
  };

  struct MAnd {
    bool is_positive;
    BinaryBuffer buf;
    ptr_type<MState> l_state, r_state;
  };

  // TODO: implement this later together with meval
  struct MAndAssign {};

  // TODO: implement this later together with meval
  struct MAndRel {};


  struct MOr {
    ptr_type<MState> l_state, r_state;
    vector<size_t> r_layout_permutation;
    BinaryBuffer buf;
    vector<event_table> eval(const database &db, size_t ts);
  };

  struct MNeg {
    ptr_type<MState> state;
    vector<event_table> eval(const database &db, size_t ts) const;
  };

  struct MExists {
    size_t idx_of_bound_var;
    ptr_type<MState> state;
    vector<event_table> eval(const database &db, size_t ts) const;
  };

  struct MPrev {
    Interval inter;
    bool is_first;
    vector<event_table> past_data;
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
    friend class monitor;
    friend struct MNeg;
    friend struct MExists;
    friend struct MOr;

    MState() = default;

  private:
    vector<event_table> eval(const database &db, size_t ts);
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

  class monitor {
  public:
    explicit monitor(const Formula &formula);
    monitor() = default;
    satisfactions step(const database &db, size_t ts);

  private:
    MState state;
    table_layout res_layout;
    size_t curr_tp{};
  };

}// namespace detail

using detail::monitor;
using detail::satisfactions;

}// namespace monitor

#endif