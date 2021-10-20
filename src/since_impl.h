#ifndef CPPMON_SINCE_IMPL_H
#define CPPMON_SINCE_IMPL_H

#include <absl/container/flat_hash_map.h>
#include <boost/container/devector.hpp>
#include <event_data.h>
#include <formula.h>
#include <monitor_types.h>
#include <table.h>
#include <vector>

namespace monitor::detail {

class since_impl_base {
protected:
  using table_buf = boost::container::devector<std::pair<size_t, event_table>>;
  using tuple_buf =
    absl::flat_hash_map<std::vector<common::event_data>, size_t>;
  // map: tuple -> (ts, counter)
  using counted_tuple_buf = absl::flat_hash_map<std::vector<common::event_data>,
                                                std::pair<size_t, size_t>>;

  since_impl_base(size_t nfvs, fo::Interval inter);
  void drop_too_old(size_t ts);
  void add_new_ts(size_t ts);
  void add_new_table(event_table &&tab_r, size_t ts);
  event_table produce_result();

  size_t nfvs;
  fo::Interval inter;
  table_buf data_prev, data_in;
  tuple_buf tuple_in;
  counted_tuple_buf tuple_since;
};

class since_impl : since_impl_base {
public:
  since_impl(bool left_negated, size_t nfvs, std::vector<size_t> comm_idx_r,
             fo::Interval inter);
  event_table eval(event_table &tab_l, event_table &tab_r, size_t new_ts);

  void print_state();

private:
  void join(event_table &tab_l);

  bool left_negated;
  std::vector<size_t> comm_idx_r;
};

class once_impl : since_impl_base {
public:
  once_impl(size_t nfvs, fo::Interval inter);
  event_table eval(event_table &tab_r, size_t new_ts);
};

}// namespace monitor::detail


#endif// CPPMON_SINCE_IMPL_H
