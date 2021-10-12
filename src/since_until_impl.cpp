#include <fmt/core.h>
#include <since_until_impl.h>

namespace monitor::detail {

m_since_impl::m_since_impl(bool right_negated, size_t nfvs,
                           std::vector<size_t> comm_idx_r, fo::Interval inter)
    : right_negated(right_negated), nfvs(nfvs), last_cleanup(0),
      comm_idx_r(std::move(comm_idx_r)), inter(inter) {}

event_table m_since_impl::eval(event_table &tab_l, event_table &tab_r,
                               size_t new_ts) {
  add_new_ts(new_ts);
  join(tab_l);
  add_new_table(std::move(tab_r), new_ts);
  if (inter.gt_upper(new_ts - last_cleanup)) {
    cleanup(last_cleanup);
    last_cleanup = new_ts;
  }
  return produce_result();
}

void m_since_impl::cleanup(size_t before_ts) {
  auto cleanup_fn = [before_ts](const auto &entry) {
    return entry.second <= before_ts;
  };
  absl::erase_if(tuple_since, cleanup_fn);
}

void m_since_impl::add_new_ts(size_t ts) {
  while (!data_in.empty()) {
    auto old_ts = data_in.front().first;
    assert(ts >= old_ts);
    if (inter.leq_upper(ts - old_ts))
      break;
    auto &tab = data_in.front();
    for (const auto &row : tab.second) {
      auto in_it = tuple_in.find(row);
      if (in_it != tuple_in.end() && in_it->second == tab.first)
        tuple_in.erase(in_it);
    }
    data_in.pop_front();
  }
  for (; !data_prev.empty() && inter.gt_upper(ts - data_prev.front().first);
       data_prev.pop_front()) {}
  while (!data_prev.empty()) {
    auto &latest = data_prev.front();
    size_t old_ts = latest.first;
    assert(old_ts <= ts);
    if (inter.lt_lower(ts - old_ts))
      break;
    for (const auto &row : latest.second) {
      auto since_it = tuple_since.find(row);
      if (since_it != tuple_since.end() && since_it->second <= old_ts)
        tuple_in.insert_or_assign(row, old_ts);
    }
    assert(data_in.empty() || old_ts >= data_in.back().first);
    data_in.push_back(std::move(latest));
    data_prev.pop_front();
  }
}

void m_since_impl::join(event_table &tab_l) {
  auto hash_set = event_table::hash_all_destructive(tab_l);
  auto erase_cond = [this, &hash_set](const auto &tup) {
    if (right_negated)
      return hash_set.contains(filter_row(comm_idx_r, tup.first));
    else
      return !hash_set.contains(filter_row(comm_idx_r, tup.first));
  };
  absl::erase_if(tuple_since, erase_cond);
  absl::erase_if(tuple_in, erase_cond);
}

void m_since_impl::add_new_table(event_table &&tab_r, size_t ts) {
  for (const auto &row : tab_r) {
    // Do not override element
    tuple_since.try_emplace(row, ts);
  }
  if (inter.contains(0)) {
    for (const auto &row : tab_r)
      tuple_in.insert_or_assign(row, ts);
    assert(data_in.empty() || ts >= data_in.back().first);
    data_in.emplace_back(ts, std::move(tab_r));
  } else {
    assert(data_prev.empty() || ts >= data_prev.back().first);
    data_prev.emplace_back(ts, std::move(tab_r));
  }
}
event_table m_since_impl::produce_result() {
  event_table tab(nfvs);
  tab.reserve(tuple_in.size());
  for (const auto &entry : tuple_in) {
    tab.add_row(entry.first);
  }
  return tab;
}
void m_since_impl::print_state() {
  fmt::print("MSINCE STATE\ncomm_idx_r: {}\ndata_prev: {}\ndata_in: "
             "{}\ntuple_since: {}\ntuple_in: {}\nEND STATE\n",
             comm_idx_r, data_prev, data_in, tuple_since, tuple_in);
}
}// namespace monitor::detail