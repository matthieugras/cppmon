#include <fmt/core.h>
#include <monitor.h>

namespace monitor::detail {
// Monitor methods
monitor::monitor(const Formula &formula) : curr_tp_(0) {
  auto [state_tmp, layout_tmp] = MState::init_mstate(formula);
#ifndef NDEBUG
  fo::fv_set layout_fvs;
  for (auto var : layout_tmp) {
    if (!layout_fvs.insert(var).second)
      throw std::runtime_error("returned table layout contains duplicates");
  }
  assert(layout_fvs == formula.fvs());
#endif
  state_ = MState(std::move(state_tmp));
  output_var_permutation_ =
    find_permutation(id_permutation(layout_tmp.size()), layout_tmp);
}

satisfactions monitor::step(const database &db, size_t ts) {
  tp_ts_map_.emplace(max_tp_, ts);
  max_tp_++;
  auto sats = state_.eval(db, ts);
  satisfactions transformed_sats;
  transformed_sats.reserve(sats.size());
  size_t new_curr_tp = curr_tp_;
  size_t n = sats.size();
  for (size_t i = 0; i < n; ++i, ++new_curr_tp) {
    auto output_tab = sats[i].make_verdicts(output_var_permutation_);
    auto it = tp_ts_map_.find(new_curr_tp);
    transformed_sats.emplace_back(it->second, new_curr_tp,
                                  std::move(output_tab));
    tp_ts_map_.erase(it);
  }
  curr_tp_ = new_curr_tp;
  return transformed_sats;
}

// MState methods
MState::MState(val_type &&state) : state(std::move(state)) {}

event_table_vec MState::eval(const database &db, size_t ts) {
  auto visitor = [&db, ts](auto &&arg) -> event_table_vec {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, MPred, MNeg, MExists, MRel, MAndRel, MAnd,
                                   MOr, MNext, MPrev, MSince, MUntil>) {
      return arg.eval(db, ts);
    } else {
      throw not_implemented_error();
    }
  };
  return var2::visit(visitor, state);
}

MState::init_pair MState::init_eq_state(const fo::Formula::eq_t &arg) {
  const auto &r = arg.l, &l = arg.r;
  const auto *lcst = l.get_if_const(), *rcst = r.get_if_const();
  if (lcst && rcst) {
    if (*lcst == *rcst)
      return {MRel{event_table::unit_table()}, {}};
    else
      return {MRel{event_table::empty_table()}, {}};
  }
  assert(l.is_var() && rcst || lcst && r.is_var());
  if (l.is_var())
    std::swap(lcst, rcst);
  return {MRel{event_table::singleton_table(*lcst)}, {0}};
}

MState::init_pair MState::init_and_join_state(const fo::Formula &phil,
                                              const fo::Formula &phir,
                                              bool right_negated) {
  auto [l_state, l_layout] = init_mstate(phil);
  auto [r_state, r_layout] = init_mstate(phir);
  if (right_negated) {
    auto info = get_anti_join_info(l_layout, r_layout);
    return {MAnd{binary_buffer(), uniq(std::move(l_state)),
                 uniq(std::move(r_state)), info},
            info.result_layout};
  } else {
    auto info = get_join_info(l_layout, r_layout);
    /*fmt::print("and join, child_l: {}, child_r: {}, my: {}\n", l_layout,
               r_layout, info.result_layout);*/
    return {MAnd{binary_buffer(), uniq(std::move(l_state)),
                 uniq(std::move(r_state)), info},
            info.result_layout};
  }
}

MState::init_pair MState::init_and_rel_state(const fo::Formula::and_t &arg) {
  const auto &phil = *arg.phil;
  const auto *phir = arg.phir.get();
  auto [rec_state, rec_layout] = init_mstate(phil);
  bool cst_neg = false;

  if (const auto *const neg_ptr = phir->inner_if_neg()) {
    cst_neg = true;
    phir = neg_ptr;
  }

  const fo::Term *t1 = nullptr, *t2 = nullptr;
  auto cst_type = MAndRel::CST_EQ;
  if (const auto *ptr = var2::get_if<fo::Formula::eq_t>(&phir->val)) {
    t1 = &ptr->l;
    t2 = &ptr->r;
  } else if (const auto *ptr2 = var2::get_if<fo::Formula::less_t>(&phir->val)) {
    cst_type = MAndRel::CST_LESS;
    t1 = &ptr2->l;
    t2 = &ptr2->r;
  } else if (const auto *ptr3 =
               var2::get_if<fo::Formula::less_eq_t>(&phir->val)) {
    cst_type = MAndRel::CST_LESS_EQ;
    t1 = &ptr3->l;
    t2 = &ptr3->r;
  } else {
    throw std::runtime_error("unknown constraint");
  }
  vector<size_t> var_2_idx(rec_layout.size());
  for (size_t i = 0; i < rec_layout.size(); ++i)
    var_2_idx[rec_layout[i]] = i;
  return {MAndRel{uniq(std::move(rec_state)), std::move(var_2_idx), *t1, *t2,
                  cst_type, cst_neg},
          std::move(rec_layout)};
}


MState::init_pair MState::init_and_state(const fo::Formula::and_t &arg) {
  const auto &phil = *arg.phil, &phir = *arg.phir;
  if (phir.is_safe_assignment(phil.fvs())) {
    throw not_implemented_error();
  } else if (phir.is_safe_formula()) {
    return init_and_join_state(*arg.phil, *arg.phir, false);
  } else if (phir.is_constraint()) {
    return init_and_rel_state(arg);
  } else if (const auto *phir_neg = phir.inner_if_neg()) {
    assert(phir_neg != nullptr);
    return init_and_join_state(*arg.phil, *phir_neg, true);
  } else {
    throw std::runtime_error("trying to initialize state with invalid and");
  }
}

MState::init_pair MState::init_pred_state(const fo::Formula::pred_t &arg) {
  absl::flat_hash_map<size_t, vector<size_t>> var_2_pred_pos;
  vector<pair<size_t, event_data>> pos_2_cst;
  for (size_t i = 0; i < arg.pred_args.size(); ++i) {
    assert(arg.pred_args[i].is_var() || arg.pred_args[i].is_const());
    if (const auto *var = arg.pred_args[i].get_if_var())
      var_2_pred_pos[*var].push_back(i);
    else if (const auto *cst = arg.pred_args[i].get_if_const())
      pos_2_cst.emplace_back(i, *cst);
  }
  table_layout lay;
  vector<vector<size_t>> var_pos;
  for (const auto &[var, pos_idxs] : var_2_pred_pos) {
    lay.push_back(var);
    var_pos.push_back(pos_idxs);
  }
  return {MPred{lay.size(), arg.pred_name, arg.pred_args, std::move(var_pos),
                std::move(pos_2_cst)},
          lay};
}

MState::init_pair MState::init_exists_state(const fo::Formula::exists_t &arg) {
  auto [rec_state, rec_layout] = init_mstate(*arg.phi);
  table_layout this_layout;
  this_layout.reserve(rec_layout.size());
  const auto it = std::find(rec_layout.cbegin(), rec_layout.cend(), 0);
  optional<size_t> drop_idx{};
  if (it != rec_layout.cend()) {
    drop_idx = static_cast<size_t>(std::distance(rec_layout.cbegin(), it));
  }
  for (size_t i = 0; i < rec_layout.size(); ++i) {
    if (!drop_idx || i != *drop_idx)
      this_layout.push_back(rec_layout[i] - 1);
  }
  return {MExists{drop_idx, uniq(std::move(rec_state))}, this_layout};
}

MState::init_pair MState::init_next_state(const fo::Formula::next_t &arg) {
  auto [rec_state, rec_layout] = init_mstate(*arg.phi);
  return {
    MNext{arg.inter, rec_layout.size(), {}, uniq(std::move(rec_state)), true},
    rec_layout};
}

MState::init_pair MState::init_prev_state(const fo::Formula::prev_t &arg) {
  auto [rec_state, rec_layout] = init_mstate(*arg.phi);
  return {MPrev{arg.inter,
                rec_layout.size(),
                event_table(),
                {},
                uniq(std::move(rec_state)),
                true},
          rec_layout};
}

MState::init_pair MState::init_mstate(const Formula &formula) {
  auto visitor1 = [](auto &&arg) -> MState::init_pair {
    using T = std::decay_t<decltype(arg)>;
    using std::is_same_v;

    if constexpr (is_same_v<T, fo::Formula::neg_t>) {
      if (arg.phi->fvs().empty()) {
        auto rec_st = init_mstate(*arg.phi).first;
        return {MNeg{uniq(std::move(rec_st))}, {}};
      } else {
        return {MRel{event_table::empty_table()}, {}};
      }
    } else if constexpr (is_same_v<T, fo::Formula::eq_t>) {
      return init_eq_state(arg);
    } else if constexpr (is_same_v<T, fo::Formula::pred_t>) {
      auto rec_state = init_pred_state(arg);
      return rec_state;
    } else if constexpr (is_same_v<T, fo::Formula::or_t>) {
      auto [l_state, l_layout] = init_mstate(*arg.phil);
      auto [r_state, r_layout] = init_mstate(*arg.phir);
      auto permutation = find_permutation(l_layout, r_layout);
      return {MOr{uniq(std::move(l_state)), uniq(std::move(r_state)),
                  permutation, binary_buffer()},
              l_layout};
    } else if constexpr (is_same_v<T, fo::Formula::and_t>) {
      auto rec_state = init_and_state(arg);
      return rec_state;
    } else if constexpr (is_same_v<T, fo::Formula::exists_t>) {
      auto rec_state = init_exists_state(arg);
      return rec_state;
    } else if constexpr (is_same_v<T, fo::Formula::prev_t>) {
      return init_prev_state(arg);
    } else if constexpr (is_same_v<T, fo::Formula::next_t>) {
      return init_next_state(arg);
    } else if constexpr (any_type_equal_v<T, fo::Formula::since_t,
                                          fo::Formula::until_t>) {
      return init_since_until(arg);
    } else if constexpr (is_same_v<T, fo::Formula::until_t>) {
      throw not_implemented_error();
    } else {
      throw std::runtime_error("not safe");
    }
  };
  return var2::visit(visitor1, formula.val);
}

optional<vector<event_data>>
MState::MPred::match(const vector<event_data> &event_args) const {
  vector<event_data> res;
  res.reserve(nfvs);
  assert(event_args.size() == pred_args.size());
  for (const auto &[pos, cst] : pos_2_cst)
    if (event_args[pos] != cst)
      return {};
  for (const auto &poss : var_pos) {
    size_t num_pos = poss.size();
    assert(num_pos > 0);
    event_data expected = event_args[poss[0]];
    for (size_t i = 1; i < num_pos; ++i)
      if (event_args[poss[i]] != expected)
        return {};
    res.push_back(std::move(expected));
  }
  return std::move(res);
}

event_table_vec MState::MPred::eval(const database &db, size_t) {
  const auto it = db.find(pred_name);
  if (it == db.end()) {
    return {event_table()};
  }
  event_table tab(nfvs);
  for (const auto &ev : it->second) {
    auto new_row = match(ev);
    if (new_row) {
      tab.add_row(std::move(*new_row));
    }
  }
  event_table_vec res(1, event_table());
  res[0] = std::move(tab);
  return res;
}

event_table_vec MState::MRel::eval(const database &, size_t) { return {tab}; }

event_table_vec MState::MNeg::eval(const database &db, size_t ts) {
  auto rec_tabs = state->eval(db, ts);
  event_table_vec res_tabs;
  res_tabs.reserve(rec_tabs.size());
  std::transform(rec_tabs.cbegin(), rec_tabs.cend(),
                 std::back_inserter(res_tabs), [](const auto &tab) {
                   if (tab.empty())
                     return event_table::unit_table();
                   else
                     return event_table::empty_table();
                 });
  return res_tabs;
}

event_table_vec MState::MExists::eval(const database &db, size_t ts) {
  auto rec_tabs = state->eval(db, ts);
  if (drop_idx)
    std::for_each(rec_tabs.begin(), rec_tabs.end(),
                  [this](auto &tab) { tab.drop_col(*drop_idx); });
  return rec_tabs;
}

event_table_vec MState::MOr::eval(const database &db, size_t ts) {
  auto reduction_fn = [this](const event_table &tab1, const event_table &tab2) {
    return tab1.t_union(tab2, r_layout_permutation);
  };
  return apply_recursive_bin_reduction(reduction_fn, *l_state, *r_state, buf,
                                       db, ts);
}

event_table_vec MState::MAnd::eval(const database &db, size_t ts) {
  auto reduction_fn = [this](const event_table &tab1,
                             const event_table &tab2) -> event_table {
    if (const auto *anti_join_ptr = var2::get_if<anti_join_info>(&op_info)) {
      return tab1.anti_join(tab2, *anti_join_ptr);
    } else {
      const auto *join_ptr = var2::get_if<join_info>(&op_info);
      return tab1.natural_join(tab2, *join_ptr);
    }
  };
  auto ret = apply_recursive_bin_reduction(reduction_fn, *l_state, *r_state,
                                           buf, db, ts);
  // fmt::print("MAND returned: {}\n", ret);
  return ret;
}

event_table_vec MState::MAndRel::eval(const database &db, size_t ts) {
  auto rec_tabs = state->eval(db, ts);
  event_table_vec res_tabs;
  res_tabs.reserve(rec_tabs.size());
  auto filter_fn = [this](const auto &row) {
    auto l_res = l.eval(var_2_idx, row), r_res = r.eval(var_2_idx, row);
    bool ret;
    if (cst_type == CST_EQ)
      ret = l_res == r_res;
    else if (cst_type == CST_LESS)
      ret = l_res < r_res;
    else
      ret = l_res <= r_res;
    if (cst_neg)
      ret = !ret;
    return ret;
  };
  std::transform(
    rec_tabs.cbegin(), rec_tabs.cend(), std::back_inserter(res_tabs),
    [filter_fn](const auto &tab) { return tab.filter(filter_fn); });
  return res_tabs;
}

event_table_vec MState::MPrev::eval(const database &db, size_t ts) {
  auto rec_tabs = state->eval(db, ts);
  past_ts.push_back(ts);
  if (rec_tabs.empty())
    return rec_tabs;
  event_table_vec res_tabs;
  res_tabs.reserve(rec_tabs.size() + 1);
  auto tabs_it = rec_tabs.begin();
  if (is_first) {
    buf = std::move(*tabs_it);
    tabs_it++;
    res_tabs.push_back(event_table(num_fvs));
    is_first = false;
  }

  if (tabs_it == rec_tabs.end())
    return res_tabs;

  assert(past_ts.size() > 1 && past_ts[0] <= past_ts[1]);
  if (inter.contains(past_ts[1] - past_ts[0]))
    res_tabs.push_back(std::move(buf));

  for (; (tabs_it + 1) < rec_tabs.end(); past_ts.pop_front(), ++tabs_it) {
    assert(past_ts.size() > 1 && past_ts[0] <= past_ts[1]);
    if (inter.contains(past_ts[1] - past_ts[0]))
      res_tabs.push_back(*tabs_it);
    else
      res_tabs.push_back(event_table(num_fvs));
  }
  buf = std::move(*tabs_it);
  return res_tabs;
}

event_table_vec MState::MNext::eval(const database &db, size_t ts) {
  auto rec_tabs = state->eval(db, ts);
  past_ts.push_back(ts);
  if (rec_tabs.empty())
    return rec_tabs;
  auto tabs_it = rec_tabs.begin();
  if (is_first) {
    tabs_it++;
    is_first = false;
  }
  event_table_vec res_tabs;
  res_tabs.reserve(rec_tabs.size());
  for (; tabs_it != rec_tabs.end(); past_ts.pop_front(), ++tabs_it) {
    assert(past_ts.size() > 1 && past_ts[0] <= past_ts[1]);
    if (inter.contains(past_ts[1] - past_ts[0]))
      res_tabs.push_back(std::move(*tabs_it));
    else
      res_tabs.push_back(event_table(num_fvs));
  }
  assert(!past_ts.empty() && tabs_it == rec_tabs.end());
  return res_tabs;
}

event_table_vec MState::MSince::eval(const database &db, size_t ts) {
  ts_buf.push_back(ts);
  auto reduction_fn = [this](event_table &tab_l,
                             event_table &tab_r) -> event_table {
    assert(!ts_buf.empty());
    size_t new_ts = ts_buf.front();
    ts_buf.pop_front();
    /*fmt::print(
      "calling since_impl eval with new ts: {}, tab_l: {}, tab_r: {}\n", new_ts,
      tab_l, tab_r);*/
    auto ret = impl.eval(tab_l, tab_r, new_ts);
    //fmt::print("since_impl returned: {}\n", ret);
    //fmt::print("since_impl state is:\n");
    //impl.print_state();
    return ret;
  };
  auto ret = apply_recursive_bin_reduction(reduction_fn, *l_state, *r_state,
                                           buf, db, ts);
  return ret;
}

event_table_vec MState::MUntil::eval(const database &db, size_t ts) {
  ts_buf.push_back(ts);
  auto reduction_fn = [this](event_table &tab_l,
                             event_table &tab_r) -> event_table_vec {
    assert(!ts_buf.empty());
    size_t new_ts = ts_buf.front();
    ts_buf.pop_front();
    // fmt::print("calling add_tables with ts {}\n", new_ts);
    impl.add_tables(tab_l, tab_r, new_ts);
    // fmt::print("state after add_tables():\n");
    // impl.print_state();
    new_ts = ts_buf.empty() ? new_ts : ts_buf.front();
    // fmt::print("calling eval with ts {}\n", new_ts);
    auto bla = impl.eval(new_ts);
    // fmt::print("state after eval():\n");
    // impl.print_state();
    return bla;
  };
  auto ret = apply_recursive_bin_reduction(reduction_fn, *l_state, *r_state,
                                           buf, db, ts);
  return ret;
}

}// namespace monitor::detail
