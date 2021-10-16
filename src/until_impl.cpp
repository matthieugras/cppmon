#include <fmt/core.h>
#include <until_impl.h>

namespace monitor::detail {
until_impl::until_impl(bool left_negated, size_t nfvs,
                           std::vector<size_t> comm_idx_r, fo::Interval inter)
    : left_negated(left_negated), contains_zero(inter.contains(0)), curr_tp(0),
      nfvs(nfvs), comm_idx_r(std::move(comm_idx_r)), inter(inter) {
  a2_map.emplace(0, a2_elem_t());
}

void until_impl::print_state() {
  fmt::print("UNTIL "
             "state\ncurr_tp:{}\nts_buf:{}\na1_map:{}\na2_map:{}\nres_acc:{}"
             "\nEND_STATE\n",
             curr_tp, ts_buf, a1_map, a2_map, res_acc);
}

event_table_vec until_impl::eval(size_t new_ts) {
  shift(new_ts);
  event_table_vec ret;
  swap(ret, res_acc);
  return ret;
}

void until_impl::update_a2_inner_map(size_t tp, const event &e,
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

void until_impl::update_a2_map(size_t new_ts, const event_table &tab_r) {
  size_t new_ts_tp;
  if (contains_zero)
    new_ts_tp = curr_tp;
  else
    new_ts_tp = new_ts - std::min((inter.get_lower() - 1), new_ts);
  assert(curr_tp >= ts_buf.size());
  size_t first_tp = curr_tp - ts_buf.size();
  for (const auto &e : tab_r) {
    if (contains_zero)
      update_a2_inner_map(curr_tp, e, new_ts_tp);
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
  }
  assert(!a2_map.contains(curr_tp + 1));
  a2_map.insert_or_assign(curr_tp + 1, a2_elem_t());
}

void until_impl::update_a1_map(const event_table &tab_l) {
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

void until_impl::add_tables(event_table &tab_l, event_table &tab_r,
                              size_t new_ts) {
  assert(ts_buf.empty() || new_ts >= ts_buf.back());
  shift(new_ts);
  update_a2_map(new_ts, tab_r);
  update_a1_map(tab_l);
  curr_tp++;
  ts_buf.push_back(new_ts);
}

event_table until_impl::table_from_filtered_map(const a2_elem_t &mapping,
                                                  size_t ts, size_t first_tp) {
  event_table res(nfvs);
  for (const auto &[e, tstp] : mapping) {
    if ((!contains_zero && ts < tstp) || (contains_zero && first_tp <= tstp)) {
      res.add_row(e);
    }
  }
  return res;
}

void until_impl::combine_max(const a2_elem_t &mapping1, a2_elem_t &mapping2) {
  for (const auto &entry : mapping1) {
    auto mapping2_it = mapping2.find(entry.first);
    if (mapping2_it == mapping2.end())
      mapping2.insert(entry);
    else
      mapping2_it->second = std::max(mapping2_it->second, entry.second);
  }
}

void until_impl::shift(size_t new_ts) {
  for (; !ts_buf.empty(); ts_buf.pop_front()) {
    size_t old_ts = ts_buf.front();
    // fmt::print("shifting old_ts: {}\n", old_ts);
    assert(new_ts >= old_ts);
    if (inter.leq_upper(new_ts - old_ts))
      break;
    // fmt::print("is in interval\n");
    assert(curr_tp >= ts_buf.size());
    size_t first_tp = curr_tp - ts_buf.size();
    // fmt::print("first tp is: {}\n", first_tp);
    auto a2_it = a2_map.find(first_tp);
    assert(a2_it != a2_map.end());
    res_acc.push_back(table_from_filtered_map(a2_it->second, old_ts, first_tp));
    auto a2_it_nxt = a2_map.find(first_tp + 1);
    assert(a2_it_nxt != a2_map.end());
    combine_max(a2_it->second, a2_it_nxt->second);
    a2_map.erase(a2_it);
  }
}
}// namespace monitor::detail