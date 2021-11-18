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
  // fmt::print("layout_fvs: {}, formula_fvs: {}\n", layout_fvs, formula.fvs());
  assert(layout_fvs == formula.fvs());
#endif
  state_ = MState(std::move(state_tmp));
  output_var_permutation_ =
    find_permutation(id_permutation(layout_tmp.size()), layout_tmp);
}

satisfactions monitor::step(const database &db, const ts_list &ts) {
  for (size_t t : ts) {
    tp_ts_map_.emplace(max_tp_, t);
    max_tp_++;
  }
  auto sats = state_.eval(db, ts);
  satisfactions transformed_sats;
  transformed_sats.reserve(sats.size());
  size_t new_curr_tp = curr_tp_;
  size_t n = sats.size();
  for (size_t i = 0; i < n; ++i, ++new_curr_tp) {
    auto output_tab = sats[i].make_verdicts(output_var_permutation_);
    auto it = tp_ts_map_.find(new_curr_tp);
    if (it->second < MAXIMUM_TIMESTAMP)
      transformed_sats.emplace_back(it->second, new_curr_tp,
                                    std::move(output_tab));
    tp_ts_map_.erase(it);
  }
  curr_tp_ = new_curr_tp;
  return transformed_sats;
}

satisfactions monitor::last_step() {
  return step({}, make_vector(static_cast<size_t>(MAXIMUM_TIMESTAMP)));
}

// MState methods
MState::MState(val_type &&state) : state(std::move(state)) {}

event_table_vec MState::eval(const database &db, const ts_list &ts) {
  auto visitor = [&db, ts](auto &&arg) -> event_table_vec {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, MPred, MNeg, MLet, MExists, MRel, MAndRel,
                                   MAndAssign, MAnd, MOr, MNext, MPrev,
                                   MSince<since_impl>, MSince<since_agg_impl>,
                                   MOnce<once_impl>, MOnce<once_agg_impl>,
                                   MUntil, MEventually, MAgg>) {
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
    return {MRel{event_table::singleton_table(*rcst)}, {*l.get_if_var()}};
  else
    return {MRel{event_table::singleton_table(*lcst)}, {*r.get_if_var()}};
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

static vector<size_t> get_sparse_var_2_idx(const table_layout &lay) {
  size_t res_size =
    lay.size() == 0 ? 0 : (*std::max_element(lay.cbegin(), lay.cend()) + 1);
  vector<size_t> var_2_idx(res_size);
  for (size_t i = 0; i < lay.size(); ++i)
    var_2_idx[lay[i]] = i;
  return var_2_idx;
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

  const fo::Term *t1, *t2;
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
  auto var_2_idx = get_sparse_var_2_idx(rec_layout);
  // fmt::print("for constraint, var_2_idx is: {}\n", var_2_idx);
  return {MAndRel{uniq(std::move(rec_state)), std::move(var_2_idx), *t1, *t2,
                  cst_type, cst_neg},
          std::move(rec_layout)};
}

MState::init_pair MState::init_and_safe_assign(const fo::Formula &phil,
                                               const fo::Formula &phir) {
  auto [rec_state, rec_layout] = init_mstate(phil);
  auto fv_l = phil.fvs();
  if (const auto *eq_ptr = var2::get_if<fo::Formula::eq_t>(&phir.val)) {
    const Term &l = eq_ptr->l, &r = eq_ptr->r;
    size_t new_var;
    const Term *trm_to_eval;
    if (l.is_var() && r.is_var()) {
      bool l_not_new_var = is_subset({*l.get_if_var()}, fv_l),
           r_not_new_var = is_subset({*r.get_if_var()}, fv_l);
      assert(l_not_new_var ^ r_not_new_var);
      if (l_not_new_var) {
        new_var = *r.get_if_var();
        trm_to_eval = &l;
      } else {
        new_var = *l.get_if_var();
        trm_to_eval = &r;
      }
    } else if (l.is_var()) {
      new_var = *l.get_if_var();
      trm_to_eval = &r;
    } else {
      assert(r.is_var());
      new_var = *r.get_if_var();
      trm_to_eval = &l;
    }
    auto var_2_idx = get_sparse_var_2_idx(rec_layout);
    rec_layout.push_back(new_var);
    // fmt::print("rec_layout and assign: {}, nfvs: {}\n", rec_layout,
    // rec_layout.size());
    return {MAndAssign{uniq(std::move(rec_state)), std::move(var_2_idx),
                       *trm_to_eval, rec_layout.size()},
            std::move(rec_layout)};
  } else {
    throw std::runtime_error("unsupported fragment");
  }
}

MState::init_pair MState::init_and_state(const fo::Formula::and_t &arg) {
  const auto &phil = *arg.phil, &phir = *arg.phir;
  if (phir.is_safe_assignment(phil.fvs())) {
    return init_and_safe_assign(*arg.phil, *arg.phir);
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
  size_t n_args = arg.pred_args.size();
  MPred::pred_ty pred_ty;
  if (arg.pred_name == "tp" && n_args == 1)
    pred_ty = MPred::TP_PRED;
  else if (arg.pred_name == "ts" && n_args == 1)
    pred_ty = MPred::TS_PRED;
  else if (arg.pred_name == "tpts" && n_args == 2)
    pred_ty = MPred::TPTS_PRED;
  else
    pred_ty = MPred::OTHER_PRED;
  absl::flat_hash_map<size_t, vector<size_t>> var_2_pred_pos;
  vector<pair<size_t, event_data>> pos_2_cst;
  for (size_t i = 0; i < n_args; ++i) {
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
  auto mpred_state = MPred{pred_ty,
                           0,
                           arg.pred_args.size(),
                           lay.size(),
                           arg.pred_name,
                           arg.pred_args,
                           std::move(var_pos),
                           std::move(pos_2_cst)};
  // mpred_state.print_state();
  return {std::move(mpred_state), lay};
}

void MState::MPred::print_state() {
  fmt::print("MPRED STATE \nnfvs: {}, pred_name: {}, var_pos: "
             "{}, pos_2_cst: {}\n",
             nfvs, pred_name, var_pos, pos_2_cst);
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
                {},
                {},
                uniq(std::move(rec_state)),
                true},
          rec_layout};
}

MState::init_pair MState::init_agg_state(const fo::Formula::agg_t &arg) {
  // TODO: clean this up - split init into two functions
  if (auto *since_ptr = var2::get_if<fo::Formula::since_t>(&arg.phi->val)) {
    auto rec_layout = init_since_until(*since_ptr, no_agg{});
    auto impl = agg_temporal::temporal_aggregation_impl(
      rec_layout.second, arg.agg_term, arg.default_value, arg.ty, arg.res_var,
      arg.num_bound_vars);
    return init_since_until(*since_ptr, std::move(impl));
  } else {
    auto [rec_state, rec_layout] = init_mstate(*arg.phi);
    auto impl =
      agg_base::aggregation_impl(rec_layout, arg.agg_term, arg.default_value,
                                 arg.ty, arg.res_var, arg.num_bound_vars);
    auto layout = impl.get_layout();
    return {MAgg{uniq(std::move(rec_state)), std::move(impl)},
            std::move(layout)};
  }
}

MState::init_pair MState::init_let_state(const fo::Formula::let_t &arg) {
  auto [phi_state, phi_layout] = init_mstate(*arg.phi);
  auto [psi_state, psi_layout] = init_mstate(*arg.psi);
  auto proj_mask =
    find_permutation(id_permutation(phi_layout.size()), phi_layout);
  return {MLet{std::move(proj_mask),
               {std::move(arg.pred_name), phi_layout.size()},
               uniq(std::move(phi_state)),
               uniq(std::move(psi_state))},
          std::move(psi_layout)};
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
      return init_since_until(arg, no_agg{});
    } else if constexpr (is_same_v<T, fo::Formula::agg_t>) {
      return init_agg_state(arg);
    } else if constexpr (is_same_v<T, fo::Formula::let_t>) {
      return init_let_state(arg);
    } else {
      throw std::runtime_error("not safe");
    }
  };
  return var2::visit(visitor1, formula.val);
}


void MState::MPred::match(const vector<event_data> &event_args,
                          event_table &acc_tab) const {
  vector<event_data> res;
  res.reserve(nfvs);
  assert(event_args.size() == pred_args.size());
  for (const auto &[pos, cst] : pos_2_cst)
    if (event_args[pos] != cst)
      return;
  for (const auto &poss : var_pos) {
    size_t num_pos = poss.size();
    assert(num_pos > 0);
    event_data expected = event_args[poss[0]];
    for (size_t i = 1; i < num_pos; ++i)
      if (event_args[poss[i]] != expected)
        return;
    res.push_back(std::move(expected));
  }
  acc_tab.add_row(std::move(res));
}

event_table_vec MState::MPred::eval(const database &db, const ts_list &ts) {
  size_t num_tps = ts.size();
  event_table_vec res_tabs;
  res_tabs.reserve(num_tps);
  if (ty == TP_PRED) {
    event_table tab(nfvs);
    for (size_t i = 0; i < num_tps; ++i, ++curr_tp)
      match(make_vector(common::event_data::Int(static_cast<int64_t>(curr_tp))),
            tab);
    res_tabs.push_back(std::move(tab));
  } else if (ty == TS_PRED) {
    event_table tab(nfvs);
    for (size_t curr_ts : ts)
      match(make_vector(common::event_data::Int(static_cast<int64_t>(curr_ts))),
            tab);
    res_tabs.push_back(std::move(tab));
  } else if (ty == TPTS_PRED) {
    event_table tab(nfvs);
    for (size_t i = 0; i < num_tps; ++i, ++curr_tp)
      match(make_vector(common::event_data::Int(static_cast<int64_t>(curr_tp)),
                        common::event_data::Int(static_cast<int64_t>(ts[i]))),
            tab);
    res_tabs.push_back(std::move(tab));
  } else {
    const auto it = db.find(std::pair(pred_name, arity));
    if (it == db.end()) {
      return {event_table(nfvs)};
    }
    for (const auto &ev_for_ts : it->second) {
      event_table tab(nfvs);
      for (const auto &ev : ev_for_ts)
        match(ev, tab);
      res_tabs.push_back(std::move(tab));
    }
  }
  return res_tabs;
}

event_table_vec MState::MRel::eval(const database &, const ts_list &ts) {
  size_t num_tabs = ts.size();
  event_table_vec res_tabs;
  res_tabs.reserve(ts.size());
  for (size_t i = 0; i < num_tabs; ++i)
    res_tabs.push_back(tab);
  return res_tabs;
}

event_table_vec MState::MNeg::eval(const database &db, const ts_list &ts) {
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

event_table_vec MState::MExists::eval(const database &db, const ts_list &ts) {
  auto rec_tabs = state->eval(db, ts);
  if (drop_idx)
    std::for_each(rec_tabs.begin(), rec_tabs.end(),
                  [this](auto &tab) { tab.drop_col(*drop_idx); });
  return rec_tabs;
}

event_table_vec MState::MOr::eval(const database &db, const ts_list &ts) {
  auto reduction_fn = [this](const event_table &tab1, const event_table &tab2) {
    return tab1.t_union(tab2, r_layout_permutation);
  };
  return apply_recursive_bin_reduction(reduction_fn, *l_state, *r_state, buf,
                                       db, ts);
}

event_table_vec MState::MAnd::eval(const database &db, const ts_list &ts) {
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

event_table_vec MState::MAndRel::eval(const database &db, const ts_list &ts) {
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
    else {
      assert(cst_type == CST_LESS_EQ);
      ret = l_res <= r_res;
    }
    if (cst_neg)
      ret = !ret;
    return ret;
  };
  std::transform(
    rec_tabs.cbegin(), rec_tabs.cend(), std::back_inserter(res_tabs),
    [filter_fn](const auto &tab) { return tab.filter(filter_fn); });
  return res_tabs;
}

event_table_vec MState::MAndAssign::eval(const database &db,
                                         const ts_list &ts) {
  auto rec_tabs = state->eval(db, ts);
  // fmt::print("MAndAssign::eval rec_tabs: {}\n", rec_tabs);
  event_table_vec res_tabs;
  res_tabs.reserve(rec_tabs.size());
  for (const auto &tab : rec_tabs) {
    event_table new_tab(nfvs);
    for (const auto &row : tab) {
      auto row_cp = row;
      auto trm_eval = t.eval(var_2_idx, row);
      // fmt::print("MAndAssign::eval trm_eval: {}\n", trm_eval);
      row_cp.push_back(std::move(trm_eval));
      // fmt::print("MAndAssign::eval new_row: {}\n", row_cp);
      new_tab.add_row(std::move(row_cp));
    }
    res_tabs.push_back(std::move(new_tab));
  }
  return res_tabs;
}

event_table_vec MState::MPrev::eval(const database &db, const ts_list &ts) {
  auto rec_tabs = state->eval(db, ts);
  past_ts.insert(past_ts.end(), ts.begin(), ts.end());
  if (rec_tabs.empty()) {
    return rec_tabs;
  }
  /*fmt::print("rec_tabs not empty, rec_tabs: {}, past_ts: {}, buf: {}\n",
             rec_tabs, past_ts, buf);*/
  event_table_vec res_tabs;
  res_tabs.reserve(rec_tabs.size() + 1);
  auto tabs_it = rec_tabs.begin();
  if (is_first) {
    res_tabs.push_back(event_table(num_fvs));
    is_first = false;
  }

  if (past_ts.size() >= 2) {
    if (buf) {
      std::optional<event_table> taken_val;
      buf.swap(taken_val);
      assert(past_ts[1] >= past_ts[0]);
      if (inter.contains(past_ts[1] - past_ts[0])) {
        res_tabs.push_back(std::move(*taken_val));
      } else {
        res_tabs.push_back(event_table(num_fvs));
      }
      past_ts.pop_front();
    }
    for (auto rec_tabs_end = rec_tabs.end();
         past_ts.size() >= 2 && tabs_it != rec_tabs_end;
         past_ts.pop_front(), ++tabs_it) {
      assert(past_ts[1] >= past_ts[0]);
      if (inter.contains(past_ts[1] - past_ts[0]))
        res_tabs.push_back(std::move(*tabs_it));
      else
        res_tabs.push_back(event_table(num_fvs));
    }
  }

  if (tabs_it != rec_tabs.end()) {
    /*fmt::print("print before assert res_tabs: {}, past_ts: {}, buf: {}\n",
       res_tabs, past_ts, buf);*/
    assert(!buf);
    buf.emplace(std::move(*tabs_it));
    tabs_it++;
  }

  assert(tabs_it == rec_tabs.end());
  /*fmt::print("returning res_tabs: {}, past_ts: {}, buf: {}\n", res_tabs,
             past_ts, buf);*/
  return res_tabs;
}

event_table_vec MState::MNext::eval(const database &db, const ts_list &ts) {
  auto rec_tabs = state->eval(db, ts);
  past_ts.insert(past_ts.end(), ts.begin(), ts.end());
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

event_table_vec MState::MUntil::eval(const database &db, const ts_list &ts) {
  ts_buf.insert(ts_buf.end(), ts.begin(), ts.end());
  auto reduction_fn = [this](event_table &tab_l,
                             event_table &tab_r) -> event_table_vec {
    assert(!ts_buf.empty());
    size_t new_ts = ts_buf.front();
    ts_buf.pop_front();
    impl.add_tables(tab_l, tab_r, new_ts);
    new_ts = ts_buf.empty() ? new_ts : ts_buf.front();
    return impl.eval(new_ts);
  };
  auto ret = apply_recursive_bin_reduction(reduction_fn, *l_state, *r_state,
                                           buf, db, ts);
  return ret;
}

event_table_vec MState::MEventually::eval(const database &db,
                                          const ts_list &ts) {
  ts_buf.insert(ts_buf.end(), ts.begin(), ts.end());
  auto rec_tabs = r_state->eval(db, ts);
  event_table_vec res_tabs;
  res_tabs.reserve(rec_tabs.size());
  for (auto &tab : rec_tabs) {
    assert(!ts_buf.empty());
    size_t new_ts = ts_buf.front();
    ts_buf.pop_front();
    impl.add_right_table(tab, new_ts);
    new_ts = ts_buf.empty() ? new_ts : ts_buf.front();
    auto res_tabs_part = impl.eval(new_ts);
    res_tabs.insert(res_tabs.end(),
                    std::make_move_iterator(res_tabs_part.begin()),
                    std::make_move_iterator(res_tabs_part.end()));
  }
  return res_tabs;
}

event_table_vec MState::MAgg::eval(const database &db, const ts_list &ts) {
  auto rec_tabs = state->eval(db, ts);
  event_table_vec res_tabs;
  res_tabs.reserve(rec_tabs.size());
  std::transform(rec_tabs.begin(), rec_tabs.end(), std::back_inserter(res_tabs),
                 [this](auto &tab) { return impl.eval(tab); });
  return res_tabs;
}

event_table_vec MState::MLet::eval(const database &db, const ts_list &ts) {
  auto l_tabs = phi_state->eval(db, ts);

  // TODO: maybe it is better not to do a hard fork and rollback changes instead
  auto db_cp = db;
  auto &db_ent = db_cp[db_idx];
  db_ent.clear();
  const size_t n_l_tabs = l_tabs.size();
  db_ent.reserve(n_l_tabs);
  for (const auto &l_tab : l_tabs) {
    parse::database_elem new_tab;
    new_tab.reserve(l_tab.tab_size());
    for (const auto &e : l_tab)
      new_tab.push_back(filter_row(projection_mask, e));
    db_ent.push_back(std::move(new_tab));
  }
  return psi_state->eval(db_cp, ts);
}

}// namespace monitor::detail
