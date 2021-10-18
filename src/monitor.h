#ifndef MFORMULA_H
#define MFORMULA_H

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <boost/container/devector.hpp>
#include <boost/variant2/variant.hpp>
#include <buffers.h>
#include <event_data.h>
#include <formula.h>
#include <iterator>
#include <monitor_types.h>
#include <optional>
#include <since_impl.h>
#include <stdexcept>
#include <string_view>
#include <table.h>
#include <traceparser.h>
#include <tuple>
#include <type_traits>
#include <until_impl.h>
#include <util.h>
#include <variant>
#include <vector>

namespace monitor {
namespace detail {
  namespace var2 = boost::variant2;
  using absl::flat_hash_map;
  using absl::flat_hash_set;
  using boost::container::devector;
  using common::event_data;
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

  class monitor;

  template<typename F, typename T>
  event_table_vec apply_recursive_bin_reduction(F f, T &t1, T &t2,
                                                binary_buffer &buf,
                                                const database &db, size_t ts) {
    auto l_rec_tabs = t1.eval(db, ts), r_rec_tabs = t2.eval(db, ts);
    auto res = buf.template update_and_reduce(l_rec_tabs, r_rec_tabs, f);
    if constexpr (std::is_same_v<decltype(res), event_table_vec>) {
      return res;
    } else {
      static_assert(
        std::is_same_v<decltype(res), std::vector<event_table_vec>>);
      event_table_vec acc;
      size_t total_size = 0;
      for (const auto &elem : res)
        total_size += elem.size();
      acc.reserve(total_size);
      for (auto &elem : res)
        acc.insert(acc.end(), std::make_move_iterator(elem.begin()),
                   std::make_move_iterator(elem.end()));
      return acc;
    }
  }


  class MState {
    friend class monitor;

    template<typename F, typename T>
    friend event_table_vec
    apply_recursive_bin_reduction(F, T &, T &, binary_buffer &,
                                  database const &, size_t);

    MState() = default;
    MState &operator=(MState &&other) = default;


  private:
    event_table_vec eval(const database &db, size_t ts);
    struct MRel {
      event_table tab;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MPred {
      size_t nfvs;
      name pred_name;
      vector<Term> pred_args;
      vector<vector<size_t>> var_pos;
      vector<pair<size_t, event_data>> pos_2_cst;
      event_table_vec eval(const database &db, size_t ts);

      [[nodiscard]] optional<event> match(const event &event_args) const;
    };

    struct MAnd {
      binary_buffer buf;
      ptr_type<MState> l_state, r_state;
      variant<join_info, anti_join_info> op_info;
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
      optional<size_t> drop_idx;
      ptr_type<MState> state;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MPrev {
      Interval inter;
      size_t num_fvs;
      event_table buf;
      devector<size_t> past_ts;
      ptr_type<MState> state;
      bool is_first;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MNext {
      Interval inter;
      size_t num_fvs;
      devector<size_t> past_ts;
      ptr_type<MState> state;
      bool is_first;
      event_table_vec eval(const database &db, size_t ts);
    };

    struct MSince {
      binary_buffer buf;
      devector<size_t> ts_buf;
      ptr_type<MState> l_state, r_state;
      since_impl impl;

      event_table_vec eval(const database &db, size_t ts);
    };

    struct MOnce {
      devector<size_t> ts_buf;
      ptr_type<MState> r_state;
      since_impl impl;

      event_table_vec eval(const database &db, size_t ts);
    };

    struct MUntil {
      binary_buffer buf;
      devector<size_t> ts_buf;
      ptr_type<MState> l_state, r_state;
      until_impl impl;

      event_table_vec eval(const database &db, size_t ts);
    };

    using val_type = variant<MRel, MPred, MOr, MExists, MPrev, MNext, MNeg,
                             MAndRel, MAndAssign, MAnd, MSince, MOnce, MUntil>;
    using init_pair = pair<val_type, table_layout>;

    explicit MState(val_type &&state);
    template<typename F>
    static inline std::unique_ptr<MState> uniq(F &&arg) {
      return std::unique_ptr<MState>(new MState(std::forward<F>(arg)));
    }

    static init_pair init_pred_state(const fo::Formula::pred_t &arg);

    static init_pair init_eq_state(const fo::Formula::eq_t &arg);

    static init_pair init_and_state(const fo::Formula::and_t &arg);

    static init_pair init_and_join_state(const fo::Formula &phil,
                                         const fo::Formula &phir,
                                         bool right_negated);
    static init_pair init_and_rel_state(const fo::Formula::and_t &arg);

    static init_pair init_exists_state(const fo::Formula::exists_t &arg);

    static init_pair init_next_state(const fo::Formula::next_t &arg);

    static init_pair init_prev_state(const fo::Formula::prev_t &arg);

    static init_pair init_since_state(const fo::Formula::since_t &arg);

    static init_pair init_once_state(const fo::Formula::since_t &arg);

    static init_pair init_until_state(const fo::Formula::until_t &arg);

    template<typename T>
    static std::enable_if_t<
      any_type_equal_v<T, fo::Formula::until_t, fo::Formula::since_t>,
      init_pair>
    init_since_until(const T &arg) {
      using St = std::conditional_t<std::is_same_v<T, fo::Formula::since_t>,
                                    MSince, MUntil>;
      using Impl = std::conditional_t<std::is_same_v<T, fo::Formula::since_t>,
                                      since_impl, until_impl>;
      auto [r_state, r_layout] = init_mstate(*arg.phir);
      ptr_type<MState> l_state;
      table_layout l_layout;
      bool left_negated = false;
      if (const auto *neg_inner = arg.phil->inner_if_neg()) {
        auto inner_neg_pair = init_mstate(*neg_inner);
        l_state = uniq(std::move(inner_neg_pair.first));
        l_layout = std::move(inner_neg_pair.second);
        left_negated = true;
      } else {
        auto pos_pair = init_mstate(*arg.phil);
        l_state = uniq(std::move(pos_pair.first));
        l_layout = std::move(pos_pair.second);
      }
      auto info = get_join_info(l_layout, r_layout);
      Impl impl(left_negated, r_layout.size(), std::move(info.comm_idx2),
                arg.inter);
      return {St{binary_buffer(),
                 {},
                 std::move(l_state),
                 uniq(std::move(r_state)),
                 impl},
              std::move(r_layout)};
    }

    static init_pair init_mstate(const Formula &formula);
    val_type state;
  };

  class monitor {
  public:
    monitor() = default;
    explicit monitor(const Formula &formula);
    satisfactions step(const database &db, size_t ts);

  private:
    MState state_;
    vector<size_t> output_var_permutation_;
    absl::flat_hash_map<size_t, size_t> tp_ts_map_;
    size_t curr_tp_{};
    size_t max_tp_{};
  };

}// namespace detail

using detail::monitor;

}// namespace monitor

#endif