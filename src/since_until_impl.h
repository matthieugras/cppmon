#ifndef CPPMON_SINCE_UNTIL_IMPL_H
#define CPPMON_SINCE_UNTIL_IMPL_H

#include <absl/container/flat_hash_map.h>
#include <boost/container/devector.hpp>
#include <event_data.h>
#include <formula.h>
#include <monitor_types.h>
#include <table.h>
#include <vector>

namespace monitor::detail {
class m_since_impl {
public:
  using table_buf = boost::container::devector<std::pair<size_t, event_table>>;
  using tuple_buf =
    absl::flat_hash_map<std::vector<common::event_data>, size_t>;
  m_since_impl(bool right_negated, size_t nfvs, std::vector<size_t> comm_idx_r,
               fo::Interval inter);
  event_table eval(event_table &tab_l, event_table &tab_r, size_t ts);

private:
  void add_new_ts(size_t ts);
  void join(event_table &tab_l);
  void add_new_table(event_table &&tab_r, size_t ts);
  event_table produce_result();
  void print_state();

  bool right_negated;
  size_t nfvs;
  std::vector<size_t> comm_idx_r;
  fo::Interval inter;
  table_buf data_prev, data_in;
  tuple_buf tuple_since, tuple_in;
};
}// namespace monitor::detail

#endif
