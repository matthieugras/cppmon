#include <fmt/core.h>
#include <until_impl.h>

namespace monitor::detail {
until_impl_base::until_impl_base(size_t nfvs, fo::Interval inter)
    : contains_zero(inter.contains(0)), first_tp(0), curr_tp(0), nfvs(nfvs),
      inter(inter) {
  a2_map.emplace(0, a2_elem_t());
}

event_table_vec until_impl_base::eval(size_t new_ts) {
  shift(new_ts);
  event_table_vec ret;
  swap(ret, res_acc);
  return ret;
}

opt_table until_impl_base::table_from_map(const a2_elem_t &mapping) {
  event_table res_tab(nfvs);
  res_tab.reserve(mapping.size());
  for (const auto &entry : mapping)
    res_tab.add_row(entry.first);
  return res_tab.empty() ? opt_table() : std::move(res_tab);
}

void until_impl_base::combine_max(a2_elem_t &mapping1, a2_elem_t &mapping2) {
  for (const auto &entry : mapping1) {
    auto mapping2_it = mapping2.find(entry.first);
    if (mapping2_it == mapping2.end())
      mapping2.insert(entry);
    else
      mapping2_it->second = std::max(mapping2_it->second, entry.second);
  }
}

void until_impl_base::update_a2_inner_map(size_t idx, const event &e,
                                          size_t new_ts_tp) {
  assert(idx < a2_map.size());
  /*auto a2_it = a2_map.find(idx);
  if (a2_it == a2_map.end()) {
    a2_map_t::mapped_type nested_map;
    nested_map.emplace(e, new_ts_tp);
    a2_map.emplace(idx, std::move(nested_map));
  } else {*/
  auto &nest_map = a2_map[idx];
  auto nest_it = nest_map.find(e);
  if (nest_it == nest_map.end())
    nest_map.emplace(e, new_ts_tp);
  else
    nest_it->second = std::max(nest_it->second, new_ts_tp);
  //}
}

void until_impl_base::shift(size_t new_ts) {
  for (; !ts_buf.empty(); ts_buf.pop_front(), a2_map.pop_front(), first_tp++) {
    size_t old_ts = ts_buf.front();
    assert(new_ts >= old_ts);
    if (inter.leq_upper(new_ts - old_ts))
      break;
    assert(a2_map.size() >= 2);
    assert(curr_tp >= ts_buf.size());
    auto erase_cond = [this, old_ts](const auto &entry) {
      size_t tstp = entry.second;
      return !((!contains_zero && old_ts < tstp) ||
               (contains_zero && first_tp <= tstp));
    };
    absl::erase_if(a2_map[0], erase_cond);
    res_acc.push_back(table_from_map(a2_map[0]));
    combine_max(a2_map[0], a2_map[1]);
  }
}


until_impl::until_impl(bool left_negated, size_t nfvs,
                       std::vector<size_t> comm_idx_r, fo::Interval inter)
    : until_impl_base(nfvs, inter), left_negated(left_negated),
      comm_idx_r(std::move(comm_idx_r)) {}

void until_impl::print_state() {
  fmt::print("UNTIL "
             "state\ncurr_tp:{}\nts_buf:{}\na1_map:{}\na2_map:{}\nres_acc:{}"
             "\nEND_STATE\n",
             curr_tp, ts_buf, a1_map, a2_map, res_acc);
}

void until_impl::update_a2_map(size_t new_ts, const opt_table &tab_r) {
  size_t new_ts_tp;
  if (contains_zero)
    new_ts_tp = curr_tp;
  else
    new_ts_tp = new_ts - std::min((inter.get_lower() - 1), new_ts);
  assert(curr_tp >= ts_buf.size());
  if (tab_r) {
    for (const auto &e: *tab_r) {
      if (contains_zero) {
        assert(!a2_map.empty());
        update_a2_inner_map(a2_map.size() - 1, e, new_ts_tp);
      }
      const auto a1_it = a1_map.find(filter_row(comm_idx_r, e));
      size_t override_idx;
      if (a1_it == a1_map.end()) {
        if (left_negated)
          override_idx = 0;
        else
          continue;
      } else {
        if (left_negated)
          override_idx =
            a1_it->second + 1 <= first_tp ? 0 : a1_it->second + 1 - first_tp;
        else
          override_idx = a1_it->second <= first_tp ? 0 : a1_it->second - first_tp;
      }
      update_a2_inner_map(override_idx, e, new_ts_tp);
    }
  }
  a2_map.emplace_back();
}

void until_impl::update_a1_map(const opt_table &tab_l) {
  if (!left_negated) {
    if (tab_l) {
      auto erase_cond = [&tab_l](const auto &entry) {
        return !tab_l->contains(entry.first);
      };
      absl::erase_if(a1_map, erase_cond);
      for (const auto &e : *tab_l) {
        if (a1_map.contains(e))
          continue;
        a1_map.emplace(e, curr_tp);
      }
    } else {
      a1_map.clear();
    }
  } else if (tab_l) {
    for (const auto &e : *tab_l)
      a1_map.insert_or_assign(e, curr_tp);
  }
}

void until_impl::add_tables(opt_table &tab_l, opt_table &tab_r,
                            size_t new_ts) {
  assert(ts_buf.empty() || new_ts >= ts_buf.back());
  shift(new_ts);
  update_a2_map(new_ts, tab_r);
  update_a1_map(tab_l);
  ts_buf.push_back(new_ts);
  curr_tp++;
}

eventually_impl::eventually_impl(size_t nfvs, fo::Interval inter)
    : until_impl_base(nfvs, inter) {}

void eventually_impl::update_a2_map(size_t new_ts, const opt_table &tab_r) {
  size_t new_ts_tp;
  if (contains_zero)
    new_ts_tp = curr_tp;
  else
    new_ts_tp = new_ts - std::min((inter.get_lower() - 1), new_ts);
  assert(curr_tp >= ts_buf.size());
  if (tab_r) {
    for (const auto &e: *tab_r) {
      assert(!a2_map.empty());
      if (contains_zero)
        update_a2_inner_map(a2_map.size() - 1, e, new_ts_tp);
      update_a2_inner_map(0, e, new_ts_tp);
    }
  }
  a2_map.emplace_back();
}

void eventually_impl::add_right_table(opt_table &tab_r, size_t new_ts) {
  assert(ts_buf.empty() || new_ts >= ts_buf.back());
  shift(new_ts);
  update_a2_map(new_ts, tab_r);
  ts_buf.push_back(new_ts);
  curr_tp++;
}
}// namespace monitor::detail
