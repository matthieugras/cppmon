#include <monitor.h>

namespace monitor::detail {
// Monitor methods
monitor::monitor(const Formula &formula) : curr_tp(0) {
  // TODO: verify that the first TP is 0
  auto [state_tmp, layout_tmp] = MState::init_mstate(formula);
  state = MState(std::move(state_tmp));
  res_layout = std::move(layout_tmp);
}

satisfactions monitor::step(const database &db, size_t ts) {
  auto sats = state.eval(db, ts);
  satisfactions transformed_sats;
  transformed_sats.reserve(sats.size());
  size_t new_curr_tp = curr_tp;
  size_t n = sats.size();
  for (size_t i = 0; i < n; ++i, ++new_curr_tp) {
    transformed_sats.emplace_back(new_curr_tp, std::move(sats[i]));
  }
  curr_tp = new_curr_tp;
  return transformed_sats;
}

// MState methods
MState::MState(val_type &&state) : state(std::move(state)) {}

event_table_vec MState::eval(const database &db, size_t ts) {
  auto visitor = [&db, ts](auto &&arg) -> event_table_vec {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, MPred, MNeg, MExists, MRel, MAndRel,
                                   MAnd>) {
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

MState::init_pair MState::init_and_join_state(const fo::Formula::and_t &arg,
                                              bool right_negated) {
  const auto &phil = *arg.phil, &phir = *arg.phir;
  auto [l_state, l_layout] = init_mstate(phil);
  auto [r_state, r_layout] = init_mstate(phir);
  auto [res_layout, join_idx_l, join_idx_r] =
    get_join_layout(l_layout, r_layout);
  return {MAnd{binary_buffer(), join_idx_l, join_idx_r,
               uniq(std::move(l_state)), uniq(std::move(r_state)),
               right_negated},
          std::move(res_layout)};
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
  } else if ((ptr = var2::get_if<fo::Formula::eq_t>(&phir->val))) {
    cst_type = MAndRel::CST_LESS;
    t1 = &ptr->l;
    t2 = &ptr->r;
  } else if ((ptr = var2::get_if<fo::Formula::eq_t>(&phir->val))) {
    cst_type = MAndRel::CST_LESS_EQ;
    t1 = &ptr->l;
    t2 = &ptr->r;
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
    return init_and_join_state(arg, false);
  } else if (phir.is_constraint()) {
    return init_and_rel_state(arg);
  } else if (const auto *phir_neg = phir.inner_if_neg()) {
    // TODO: fix the get_join_layout function to support anti_joins
    return init_and_join_state(arg, true);
  } else {
    throw std::runtime_error("trying to initialize state with invalid and");
  }
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
      vector<size_t> tab_layout;
      for (const auto &pred_arg : arg.pred_args) {
        assert(pred_arg.is_var() || pred_arg.is_const());
        if (const auto *var = pred_arg.get_if_var()) {
          tab_layout.push_back(*var);
        }
      }
      return {MPred{arg.pred_name, arg.pred_args, tab_layout.size()},
              tab_layout};
    } else if constexpr (is_same_v<T, fo::Formula::or_t>) {
      throw not_implemented_error();
      auto [l_state, l_layout] = init_mstate(*arg.phil);
      auto [r_state, r_layout] = init_mstate(*arg.phir);
      auto permutation = find_permutation(l_layout, r_layout);
      return {MOr{uniq(std::move(l_state)), uniq(std::move(r_state)),
                  permutation, binary_buffer()},
              l_layout};
    } else if constexpr (is_same_v<T, fo::Formula::and_t>) {
      throw not_implemented_error();
      // return make_and_state(arg, n_bound_vars);
    } else if constexpr (is_same_v<T, fo::Formula::exists_t>) {
      auto [rec_state, rec_layout] = init_mstate(*arg.phi);
      table_layout this_layout;
      this_layout.reserve(rec_layout.size());
      size_t drop_idx =
        std::distance(rec_layout.cbegin(),
                      std::find(rec_layout.cbegin(), rec_layout.cend(), 0));
      for (size_t i = 0; i < rec_layout.size(); ++i) {
        if (i != drop_idx)
          this_layout.push_back(rec_layout[i] - 1);
      }
      return {MExists{drop_idx, uniq(std::move(rec_state))}, this_layout};
    } else if constexpr (is_same_v<T, fo::Formula::prev_t>) {
      throw not_implemented_error();
    } else if constexpr (is_same_v<T, fo::Formula::next_t>) {
      throw not_implemented_error();
    } else if constexpr (is_same_v<T, fo::Formula::since_t>) {
      throw not_implemented_error();
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
  res.reserve(n_fvs);
  assert(event_args.size() == pred_args.size());
  size_t n_arg = pred_args.size();
  for (size_t i = 0; i < n_arg; ++i) {
    assert(pred_args[i].is_const() || pred_args[i].is_var());
    if (const auto *ptr = pred_args[i].get_if_const()) {
      if ((*ptr) != event_args[i])
        return {};
    }
    res.push_back(event_args[i]);
  }
  return std::move(res);
}

event_table_vec MState::MPred::eval(const database &db, size_t) {
  const auto it = db.find(pred_name);
  if (it == db.end())
    return {event_table()};
  event_table tab;
  for (const auto &ev : it->second) {
    auto new_row = match(ev);
    if (new_row)
      tab.add_row(std::move(*new_row));
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
  std::for_each(rec_tabs.begin(), rec_tabs.end(),
                [this](auto &tab) { tab.drop_col(idx_of_bound_var); });
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
  auto reduction_fn = [this](const event_table &tab1, const event_table &tab2) {
    if (is_right_negated)
      return tab1.anti_join(tab2, join_idx_l, join_idx_r);
    else
      return tab1.natural_join(tab2, join_idx_l, join_idx_r);
  };
  return apply_recursive_bin_reduction(reduction_fn, *l_state, *r_state, buf,
                                       db, ts);
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

}// namespace monitor::detail
