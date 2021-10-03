#ifndef MFORMULA_H
#define MFORMULA_H

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <binary_buffer.h>
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
  using event_table_vec = vector<event_table>;
  using satisfactions = vector<pair<size_t, event_table>>;
  using database = parse::database;
  using binary_buffer = common::binary_buffer<event_table>;
  class monitor;

  // TODO: implement this later together with meval
  struct MSaux {};

  // TODO: implement this later together with meval
  struct MUaux {};

  template<typename F, typename T>
  event_table_vec apply_recursive_bin_reduction(F f, T &t1, T &t2,
                                                binary_buffer &buf,
                                                const database &db, size_t ts) {
    auto l_rec_tabs = t1.eval(db, ts), r_rec_tabs = t2.eval(db, ts);
    return buf.template update_and_reduce(l_rec_tabs, r_rec_tabs, f);
  }


  class MState {
    friend class monitor;

    template<typename F, typename T>
    friend event_table_vec
    apply_recursive_bin_reduction(F f, T &t1, T &t2, binary_buffer &buf,
                                  const database &db, size_t ts);

    MState() = default;
    MState& operator=(MState &&other) = default;


  private:
    event_table_vec eval(const database &db, size_t ts);
    struct MRel {
      event_table tab;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MPred {
      name pred_name;
      vector<Term> pred_args;
      size_t n_fvs;
      event_table_vec eval(const database &db, size_t ts);
      optional<vector<event_data>>
      match(const vector<event_data> &event_args) const;
    };

    struct MAnd {
      binary_buffer buf;
      vector<size_t> join_idx_l, join_idx_r;
      ptr_type<MState> l_state, r_state;
      bool is_right_negated;
      event_table_vec eval(const database &db, size_t ts);
    };

    // TODO: implement this later together with meval
    struct MAndAssign {};

    struct MAndRel {
      ptr_type<MState> state;
      vector<size_t> var_2_idx;
      fo::Term l, r;
      enum cst_type_t
      {
        CST_EQ,
        CST_LESS,
        CST_LESS_EQ
      } cst_type;
      bool cst_neg;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MOr {
      ptr_type<MState> l_state, r_state;
      vector<size_t> r_layout_permutation;
      binary_buffer buf;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MNeg {
      ptr_type<MState> state;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MExists {
      size_t idx_of_bound_var;
      ptr_type<MState> state;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MPrev {
      Interval inter;
      bool is_first;
      event_table_vec past_data;
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
    using val_type = variant<MRel, MPred, MOr, MExists, MPrev, MNext, MNeg,
                             MAndRel, MAndAssign, MAnd>;
    using init_pair = pair<val_type, table_layout>;
    explicit MState(val_type &&state);
    template<typename F>
    static inline std::unique_ptr<MState> uniq(F &&arg) {
      return std::unique_ptr<MState>(new MState(std::forward<F>(arg)));
    }
    static init_pair init_eq_state(const fo::Formula::eq_t &arg);

    static init_pair init_and_state(const fo::Formula::and_t &arg);

    static init_pair init_and_join_state(const fo::Formula::and_t &arg,
                                         bool right_negated);
    static init_pair init_and_rel_state(const fo::Formula::and_t &arg);

    static init_pair init_mstate(const Formula &formula);
    val_type state;
  };

  class monitor {
  public:
    monitor() = default;
    explicit monitor(const Formula &formula);
    satisfactions step(const database &db, size_t ts);

  private:
    MState state;
    table_layout res_layout;
    size_t curr_tp{};
  };

}// namespace detail

using detail::monitor;
using detail::satisfactions;
using detail::event_table;

}// namespace monitor

#endif