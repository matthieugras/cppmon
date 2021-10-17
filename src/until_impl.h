#ifndef CPPMON_UNTIL_IMPL_H
#define CPPMON_UNTIL_IMPL_H

#include <absl/container/flat_hash_map.h>
#include <absl/container/node_hash_map.h>
#include <boost/container/devector.hpp>
#include <event_data.h>
#include <formula.h>
#include <monitor_types.h>
#include <table.h>
#include <vector>

namespace monitor::detail {


class until_impl {
public:
  until_impl(bool left_negated, size_t nfvs, std::vector<size_t> comm_idx_r,
               fo::Interval inter);
  event_table_vec eval(size_t new_ts);
  void add_tables(event_table &tab_l, event_table &tab_r, size_t new_ts);
  void print_state();

private:
  using tuple_t = std::vector<common::event_data>;
  using ts_buf_t = boost::container::devector<size_t>;
  using a1_map_t = absl::flat_hash_map<tuple_t, size_t>;
  using a2_elem_t = a1_map_t;
  using a2_map_t = absl::flat_hash_map<size_t, a2_elem_t>;

  void combine_max(a2_elem_t &mapping1, a2_elem_t &mapping2);
  void update_a2_inner_map(size_t override_tp, const event &e,
                           size_t new_ts_tp);
  void update_a2_map(size_t new_ts, const event_table &tab_r);
  void update_a1_map(const event_table &tab_l);
  event_table table_from_map(const a2_elem_t &mapping);

  void shift(size_t new_ts);

  bool left_negated, contains_zero;
  size_t curr_tp, nfvs;
  std::vector<size_t> comm_idx_r;
  fo::Interval inter;
  ts_buf_t ts_buf;
  a1_map_t a1_map;
  // I don't think think that a2_map has to be a map if we keep track
  // of the lowest index
  a2_map_t a2_map;
  event_table_vec res_acc;
};
}// namespace monitor::detail

#endif// CPPMON_UNTIL_IMPL_H
