#ifndef MFORMULA_H
#define MFORMULA_H

#include "database.h"
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <aggregation_impl.h>
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
#include <temporal_aggregation_impl.h>
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
  event_table_vec
  apply_recursive_bin_reduction(F f, T &t1, T &t2, binary_buffer &buf,
                                database &db, const ts_list &ts) {
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
                                  database &, ts_list const &);

    MState() = default;
    MState &operator=(MState &&other) = default;


  private:
    event_table_vec eval(database &db, const ts_list &ts);
    struct MRel {
      event_table tab;
      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MPred {
      enum pred_ty
      {
        TP_PRED,
        TS_PRED,
        TPTS_PRED,
        OTHER_PRED
      } ty;
      size_t curr_tp, arity, nfvs;
      name pred_name;
      vector<Term> pred_args;
      vector<vector<size_t>> var_pos;
      vector<pair<size_t, event_data>> pos_2_cst;
      event_table_vec eval(database &db, const ts_list &ts);
      void match(const event &event_args, event_table &acc_tab) const;
      void print_state();
    };

    struct MAnd {
      binary_buffer buf;
      ptr_type<MState> l_state, r_state;
      variant<join_info, anti_join_info> op_info;
      event_table_vec eval(database &db, const ts_list &ts);
    };


    struct MAndAssign {
      ptr_type<MState> state;
      vector<size_t> var_2_idx;
      fo::Term t;
      size_t nfvs;
      event_table_vec eval(database &db, const ts_list &ts);
    };

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
      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MOr {
      ptr_type<MState> l_state, r_state;
      vector<size_t> r_layout_permutation;
      size_t nfvs_l;
      binary_buffer buf;
      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MNeg {
      ptr_type<MState> state;
      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MExists {
      optional<size_t> drop_idx;
      ptr_type<MState> state;
      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MPrev {
      Interval inter;
      size_t num_fvs;
      std::optional<opt_table> buf;
      devector<size_t> past_ts;
      ptr_type<MState> state;
      bool is_first;
      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MNext {
      Interval inter;
      size_t num_fvs;
      devector<size_t> past_ts;
      ptr_type<MState> state;
      bool is_first;
      event_table_vec eval(database &db, const ts_list &ts);
    };

    template<typename Impl>
    struct MSince {
      binary_buffer buf;
      devector<size_t> ts_buf;
      ptr_type<MState> l_state, r_state;
      Impl impl;

      event_table_vec eval(database &db, const ts_list &ts) {
        ts_buf.insert(ts_buf.end(), ts.begin(), ts.end());
        auto reduction_fn = [this](opt_table &tab_l,
                                   opt_table  &tab_r) -> opt_table  {
          assert(!ts_buf.empty());
          size_t new_ts = ts_buf.front();
          ts_buf.pop_front();
          //fmt::print("calling impl with tabl: {}, tabr: {}, ts: {}\n", tab_l, tab_r, new_ts);
          auto ret = impl.eval(tab_l, tab_r, new_ts);
          /*fmt::print("state after call:\n");
          impl.print_state();*/
          return ret;
        };
        auto ret = apply_recursive_bin_reduction(reduction_fn, *l_state,
                                                 *r_state, buf, db, ts);
        return ret;
      }
    };

    template<typename Impl>
    struct MOnce {
      devector<size_t> ts_buf;
      ptr_type<MState> r_state;
      Impl impl;

      event_table_vec eval(database &db, const ts_list &ts) {
        ts_buf.insert(ts_buf.end(), ts.begin(), ts.end());
        auto rec_tabs = r_state->eval(db, ts);
        event_table_vec res;
        res.reserve(rec_tabs.size());
        for (auto &tab : rec_tabs) {
          assert(!ts_buf.empty());
          size_t new_ts = ts_buf.front();
          ts_buf.pop_front();
          //fmt::print("calling impl with tabl: {}, ts: {}\n", tab, new_ts);
          auto ret = impl.eval(tab, new_ts);
          /*fmt::print("state after call:\n");
          impl.print_state();*/
          res.push_back(std::move(ret));
        }
        return res;
      }
    };

    struct MUntil {
      binary_buffer buf;
      devector<size_t> ts_buf;
      ptr_type<MState> l_state, r_state;
      until_impl impl;

      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MEventually {
      devector<size_t> ts_buf;
      ptr_type<MState> r_state;
      eventually_impl impl;

      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MAgg {
      ptr_type<MState> state;
      agg_base::aggregation_impl impl;

      event_table_vec eval(database &db, const ts_list &ts);
    };

    struct MLet {
      std::vector<size_t> projection_mask;
      std::pair<std::string, size_t> db_idx;
      ptr_type<MState> phi_state, psi_state;

      event_table_vec eval(database &db, const ts_list &ts);
    };

    using val_type = variant<MRel, MPred, MOr, MExists, MPrev, MNext, MNeg,
                             MAndRel, MAndAssign, MAnd, MSince<since_agg_impl>,
                             MSince<since_impl>, MOnce<once_agg_impl>,
                             MOnce<once_impl>, MUntil, MEventually, MAgg, MLet>;
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

    static init_pair init_and_safe_assign(const fo::Formula &phil,
                                          const fo::Formula &phir);

    static init_pair init_exists_state(const fo::Formula::exists_t &arg);

    static init_pair init_next_state(const fo::Formula::next_t &arg);

    static init_pair init_prev_state(const fo::Formula::prev_t &arg);

    static init_pair init_agg_state(const fo::Formula::agg_t &arg);

    static init_pair init_let_state(const fo::Formula::let_t &arg);


    struct no_agg {};

    template<typename T, typename Agg = no_agg>
    static std::enable_if_t<
      any_type_equal_v<T, fo::Formula::until_t, fo::Formula::since_t>,
      init_pair>
    init_since_until(const T &arg, Agg &&agg_impl) {
      using std::conditional_t;
      constexpr bool cond = std::is_same_v<T, fo::Formula::since_t>;
      constexpr bool is_in_agg = std::negation_v<std::is_same<Agg, no_agg>>;
      using Impl = conditional_t<
        cond, conditional_t<is_in_agg, since_agg_impl, since_impl>, until_impl>;
      using ImplTrue =
        conditional_t<cond, conditional_t<is_in_agg, once_agg_impl, once_impl>,
                      eventually_impl>;
      using St = conditional_t<cond, MSince<Impl>, MUntil>;
      using StTrue = conditional_t<cond, MOnce<ImplTrue>, MEventually>;

      auto [r_state, r_layout] = init_mstate(*arg.phir);
      if (arg.phil->is_always_true()) {
        if constexpr (is_in_agg) {
          auto agg_layout = agg_impl.get_layout();
          auto impl =
            ImplTrue(r_layout.size(), arg.inter, std::forward<Agg>(agg_impl));
          return {StTrue{{}, uniq(std::move(r_state)), std::move(impl)},
                  agg_layout};
        } else {
          auto impl = ImplTrue(r_layout.size(), arg.inter);
          return {StTrue{{}, uniq(std::move(r_state)), std::move(impl)},
                  r_layout};
        }
      }
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
      if constexpr (is_in_agg) {
        auto info = get_join_info(l_layout, r_layout);
        auto agg_layout = agg_impl.get_layout();
        Impl impl(left_negated, r_layout.size(), std::move(info.comm_idx2),
                  arg.inter, std::forward<Agg>(agg_impl));
        return {St{binary_buffer(),
                   {},
                   std::move(l_state),
                   uniq(std::move(r_state)),
                   std::move(impl)},
                std::move(agg_layout)};
      } else {
        auto info = get_join_info(l_layout, r_layout);
        Impl impl(left_negated, r_layout.size(), std::move(info.comm_idx2),
                  arg.inter);
        return {St{binary_buffer(),
                   {},
                   std::move(l_state),
                   uniq(std::move(r_state)),
                   std::move(impl)},
                std::move(r_layout)};
      }
    }

    static init_pair init_mstate(const Formula &formula);
    val_type state;
  };

  class monitor {
  public:
    monitor() = default;
    explicit monitor(const Formula &formula);
    satisfactions step(database &db, const ts_list &ts);
    satisfactions last_step();

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