#include <fmt/core.h>
#include <since_until_impl.h>

namespace monitor::detail {

m_since_impl::m_since_impl(bool left_negated, size_t nfvs,
                           std::vector<size_t> comm_idx_r, fo::Interval inter)
    : left_negated(left_negated), nfvs(nfvs), last_cleanup(0),
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

void m_since_impl::drop_too_old(size_t ts) {
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
    }
  }
  for (; !data_prev.empty() && inter.gt_upper(ts - data_prev.front().first);
       data_prev.pop_front()) {}
}

void m_since_impl::add_new_ts(size_t ts) {
  drop_too_old(ts);
  for (; !data_prev.empty(); data_prev.pop_front()) {
    auto &latest = data_prev.front();
    size_t old_ts = latest.first;
    assert(old_ts <= ts);
    if (inter.lt_lower(ts - old_ts))
      break;
    for (const auto &e : latest.second) {
      auto since_it = tuple_since.find(e);
      if (since_it != tuple_since.end() && since_it->second <= old_ts)
        tuple_in.insert_or_assign(e, old_ts);
    }
    assert(data_in.empty() || old_ts >= data_in.back().first);
    data_in.push_back(std::move(latest));
  }
}

void m_since_impl::join(event_table &tab_l) {
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

void m_since_impl::add_new_table(event_table &&tab_r, size_t ts) {
  for (const auto &e : tab_r)
    tuple_since.try_emplace(e, ts);
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

event_table m_since_impl::produce_result() {
  event_table tab(nfvs);
  tab.reserve(tuple_in.size());
  for (const auto &kv : tuple_in)
    tab.add_row(kv.first);
  return tab;
}
void m_since_impl::print_state() {
  fmt::print("MSINCE STATE\ncomm_idx_r: {}\ndata_prev: {}\ndata_in: "
             "{}\ntuple_since: {}\ntuple_in: {}\nEND STATE\n",
             comm_idx_r, data_prev, data_in, tuple_since, tuple_in);
}

m_until_impl::m_until_impl(bool left_negated, size_t nfvs,
                           std::vector<size_t> comm_idx_r, fo::Interval inter)
    : left_negated(left_negated), contains_zero(inter.contains(0)), curr_tp(0),
      nfvs(nfvs), comm_idx_r(std::move(comm_idx_r)), inter(inter) {
  a2_map.emplace(0, a2_elem_t());
}

event_table_vec m_until_impl::eval(size_t new_ts) {
  shift(new_ts);
  event_table_vec ret;
  swap(ret, res_acc);
  return ret;
}

void m_until_impl::update_a2_inner_map(size_t tp, const event &e,
                                       size_t new_ts_tp) {
  auto a2_it = a2_map.find(tp);
  if (a2_it == a2_map.end()) {
    a2_map_t::mapped_type nested_map;
    nested_map.emplace(e, new_ts_tp);
    a2_map.emplace(tp, std::move(nested_map));
  } else {
    auto nest_it = a2_it->second.find(e);
    if (nest_it == a2_it->second.end())
      a2_it->second.emplace(e, new_ts_tp);
    else
      nest_it->second = std::max(nest_it->second, new_ts_tp);
  }
}

void m_until_impl::update_a2_map(size_t new_ts, const event_table &tab_r) {
  size_t new_ts_tp;
  if (contains_zero) {
    new_ts_tp = curr_tp;
  } else {
    assert(new_ts >= (inter.get_lower() - 1));
    new_ts_tp = new_ts - (inter.get_lower() - 1);
  }
  size_t first_tp = curr_tp - ts_buf.size();
  for (const auto &e : tab_r) {
    const auto a1_it = a1_map.find(filter_row(comm_idx_r, e));
    size_t override_tp;
    if (a1_it == a1_map.end()) {
      if (left_negated)
        override_tp = first_tp;
      else
        continue;
    } else {
      if (left_negated)
        override_tp = std::max(first_tp, a1_it->second + 1);
      else
        override_tp = std::max(first_tp, a1_it->second);
    }
    update_a2_inner_map(override_tp, e, new_ts_tp);
    if (contains_zero)
      update_a2_inner_map(curr_tp, e, new_ts_tp);
  }
  a2_map.insert_or_assign(curr_tp + 1, {});
}

void m_until_impl::update_a1_map(const event_table &tab_l) {
  if (!left_negated) {
    auto erase_cond = [&tab_l](const auto &entry) {
      return !tab_l.contains(entry.first);
    };
    absl::erase_if(a1_map, erase_cond);
    for (const auto &e : tab_l) {
      if (a1_map.contains(e))
        continue;
      a1_map.emplace(e, curr_tp);
    }
  } else {
    for (const auto &e : tab_l)
      a1_map.insert_or_assign(e, curr_tp);
  }
}

void m_until_impl::add_tables(event_table &tab_l, event_table &tab_r,
                              size_t new_ts) {
  assert(ts_buf.empty() || new_ts >= ts_buf.back());
  shift(new_ts);
  update_a2_map(new_ts, tab_r);
  update_a1_map(tab_l);
  curr_tp++;
  ts_buf.push_back(new_ts);
}

event_table m_until_impl::table_from_filtered_map(const a2_elem_t &mapping,
                                                  size_t ts, size_t first_tp) {
  event_table res(nfvs);
  for (const auto &[e, tstp] : mapping) {
    if ((!contains_zero && ts < tstp) || (contains_zero && first_tp <= tstp)) {
      res.add_row(e);
    }
  }
  return res;
}

void m_until_impl::combine_max(const a2_elem_t &mapping1, a2_elem_t &mapping2) {
  for (const auto &entry : mapping1) {
    auto mapping2_it = mapping2.find(entry.first);
    if (mapping2_it == mapping2.end())
      mapping2.insert(entry);
    else
      mapping2_it->second = std::max(mapping2_it->second, entry.second);
  }
}

void m_until_impl::shift(size_t new_ts) {
  for (; !ts_buf.empty(); ts_buf.pop_front()) {
    size_t old_ts = ts_buf.front();
    assert(new_ts >= old_ts);
    if (inter.leq_upper(new_ts - old_ts))
      break;
    size_t first_tp = curr_tp - ts_buf.size();
    auto a2_it = a2_map.find(first_tp);
    assert(a2_it != a2_map.end());
    res_acc.push_back(table_from_filtered_map(a2_it->second, new_ts, first_tp));
    auto a2_it_nxt = a2_map.find(first_tp + 1);
    assert(a2_it_nxt != a2_map.end());
    combine_max(a2_it->second, a2_it_nxt->second);
    a2_map.erase(a2_it);
  }
}
}// namespace monitor::detail