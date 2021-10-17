#include "since_impl.h"

namespace monitor::detail {
since_impl::since_impl(bool left_negated, size_t nfvs,
                       std::vector<size_t> comm_idx_r, fo::Interval inter)
    : left_negated(left_negated), nfvs(nfvs), comm_idx_r(std::move(comm_idx_r)), inter(inter) {}

event_table since_impl::eval(event_table &tab_l, event_table &tab_r,
                             size_t new_ts) {
  add_new_ts(new_ts);
  join(tab_l);
  add_new_table(std::move(tab_r), new_ts);
  return produce_result();
}

void since_impl::drop_too_old(size_t ts) {
  for (; !data_in.empty(); data_in.pop_front()) {
    auto old_ts = data_in.front().first;
    assert(ts >= old_ts);
    if (inter.leq_upper(ts - old_ts))
      break;
    auto &tab = data_in.front();
    for (const auto &e : tab.second) {
      auto in_it = tuple_in.find(e);
      if (in_it != tuple_in.end() && in_it->second == tab.first)
        tuple_in.erase(in_it);
      auto since_it = tuple_since.find(e);
      assert(since_it == tuple_since.end() || since_it->second.second > 0);
      if (since_it != tuple_since.end() && ((--since_it->second.second) == 0))
        tuple_since.erase(since_it);
    }
  }
  for (; !data_prev.empty() && inter.gt_upper(ts - data_prev.front().first);
       data_prev.pop_front()) {
    for (const auto &e : data_prev.front().second) {
      auto since_it = tuple_since.find(e);
      assert(since_it == tuple_since.end() || since_it->second.second > 0);
      if (since_it != tuple_since.end() && ((--since_it->second.second) == 0))
        tuple_since.erase(since_it);
    }
  }
}

void since_impl::add_new_ts(size_t ts) {
  drop_too_old(ts);
  for (; !data_prev.empty(); data_prev.pop_front()) {
    auto &latest = data_prev.front();
    size_t old_ts = latest.first;
    assert(old_ts <= ts);
    if (inter.lt_lower(ts - old_ts))
      break;
    for (const auto &e : latest.second) {
      auto since_it = tuple_since.find(e);
      if (since_it != tuple_since.end() && since_it->second.first <= old_ts)
        tuple_in.insert_or_assign(e, old_ts);
    }
    assert(data_in.empty() || old_ts >= data_in.back().first);
    data_in.push_back(std::move(latest));
  }
}

void since_impl::join(event_table &tab_l) {
  auto hash_set = event_table::hash_all_destructive(tab_l);
  auto erase_cond = [this, &hash_set](const auto &tup) {
    if (left_negated)
      return hash_set.contains(filter_row(comm_idx_r, tup.first));
    else
      return !hash_set.contains(filter_row(comm_idx_r, tup.first));
  };
  absl::erase_if(tuple_since, erase_cond);
  absl::erase_if(tuple_in, erase_cond);
}

void since_impl::add_new_table(event_table &&tab_r, size_t ts) {
  for (const auto &e : tab_r) {
    auto since_it = tuple_since.find(e);
    if (since_it == tuple_since.end())
      tuple_since.emplace(e, std::pair(ts, 1));
    else
      since_it->second.second++;
  }
  if (inter.contains(0)) {
    for (const auto &e : tab_r)
      tuple_in.insert_or_assign(e, ts);
    assert(data_in.empty() || ts >= data_in.back().first);
    data_in.emplace_back(ts, std::move(tab_r));
  } else {
    assert(data_prev.empty() || ts >= data_prev.back().first);
    data_prev.emplace_back(ts, std::move(tab_r));
  }
}

event_table since_impl::produce_result() {
  event_table tab(nfvs);
  tab.reserve(tuple_in.size());
  for (const auto &kv : tuple_in)
    tab.add_row(kv.first);
  return tab;
}

void since_impl::print_state() {
  fmt::print("MSINCE STATE\ncomm_idx_r: {}\ndata_prev: {}\ndata_in: "
             "{}\ntuple_since: {}\ntuple_in: {}\nEND STATE\n",
             comm_idx_r, data_prev, data_in, tuple_since, tuple_in);
}
}// namespace monitor::detail
