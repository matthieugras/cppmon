#include <monitor.h>

namespace monitor::detail {
// Monitor methods
monitor::monitor(const Formula &formula) : curr_tp(0) {
  // TODO: verify that the first TP is 0
  auto [state_tmp, layout_tmp] = MState::make_formula_state(formula, 0);
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

vector<event_table> MState::eval(const database &db, size_t ts) {
  auto visitor = [&db, ts](auto &&arg) -> vector<event_table> {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, MPred, MNeg, MExists, MRel>) {
      return arg.eval(db, ts);
    } else {
      throw not_implemented_error();
    }
  };
  return var2::visit(visitor, state);
}

pair<MState::val_type, table_layout>
MState::make_eq_state(const fo::Formula::eq_t &arg) {
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

pair<MState::val_type, table_layout>
MState::make_and_state(const fo::Formula::and_t &arg, size_t n_bound_vars) {
  throw not_implemented_error();
  const auto &phil = arg.phil, &phir = arg.phir;
  if (phir->is_safe_assignment(phil->fvs())) {
    throw not_implemented_error();
    /*return MAndRel{};*/
  } else if (phir->is_safe_formula()) {
    throw not_implemented_error();
    /*auto l_state = MState(make_formula_state(*phil, n_bound_vars)),
         r_state = MState(make_formula_state(*phir, n_bound_vars));
    return MAnd{true, MBuf2{}, uniq(std::move(l_state)),
                uniq(std::move(r_state))};*/
  } else if (phir->is_constraint()) {
    throw not_implemented_error();
    // return MAndRel{};
  } else if (const auto *phir_neg =
               var2::get_if<fo::Formula::neg_t>(&phir->val)) {
    throw not_implemented_error();
    /*
    auto l_state = MState(make_formula_state(*phil, n_bound_vars)),
         r_state = MState(make_formula_state(*phir_neg->phi, n_bound_vars));
    return MAnd{false, MBuf2{}, uniq(std::move(l_state)),
                uniq(std::move(r_state))};*/
  } else {
    throw std::runtime_error("trying to initialize state with invalid and");
  }
}

pair<MState::val_type, table_layout>
MState::make_formula_state(const Formula &formula, size_t n_bound_vars) {
  auto visitor1 = [n_bound_vars](auto &&arg) -> pair<val_type, table_layout> {
    using T = std::decay_t<decltype(arg)>;
    using std::is_same_v;

    if constexpr (is_same_v<T, fo::Formula::neg_t>) {
      if (arg.phi->fvs().empty()) {
        auto rec_st = make_formula_state(*arg.phi, n_bound_vars).first;
        return {MNeg{uniq(MState(std::move(rec_st)))}, {}};
      } else {
        return {MRel{event_table::empty_table()}, {}};
      }
    } else if constexpr (is_same_v<T, fo::Formula::eq_t>) {
      return make_eq_state(arg);
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
      auto [l_state, l_layout] = make_formula_state(*arg.phil, n_bound_vars);
      auto [r_state, r_layout] = make_formula_state(*arg.phir, n_bound_vars);
      auto permutation = find_permutation(l_layout, r_layout);
      return {MOr{uniq(MState(std::move(l_state))),
                  uniq(MState(std::move(r_state))), permutation,
                  BinaryBuffer()},
              l_layout};
    } else if constexpr (is_same_v<T, fo::Formula::and_t>) {
      throw not_implemented_error();
      // return make_and_state(arg, n_bound_vars);
    } else if constexpr (is_same_v<T, fo::Formula::exists_t>) {
      auto [rec_state, rec_layout] =
        make_formula_state(*arg.phi, n_bound_vars + 1);
      table_layout this_layout;
      this_layout.reserve(rec_layout.size());
      size_t drop_idx =
        std::distance(rec_layout.cbegin(),
                      std::find(rec_layout.cbegin(), rec_layout.cend(), 0));
      for (size_t i = 0; i < rec_layout.size(); ++i) {
        if (i != drop_idx)
          this_layout.push_back(rec_layout[i] - 1);
      }
      return {MExists{drop_idx, uniq(MState(std::move(rec_state)))},
              this_layout};
    } else if constexpr (is_same_v<T, fo::Formula::prev_t>) {
      throw not_implemented_error();
      /*auto new_state = uniq(MState(make_formula_state(*arg.phi,
      n_bound_vars))); return MPrev{arg.inter, true, {}, {},
      std::move(new_state)};*/
    } else if constexpr (is_same_v<T, fo::Formula::next_t>) {
      throw not_implemented_error();
      /*auto new_state = uniq(MState{make_formula_state(*arg.phi,
      n_bound_vars)}); return MNext{arg.inter, true, {},
      std::move(new_state)};*/
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
MPred::match(const vector<event_data> &event_args) const {
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

vector<event_table> MPred::eval(const database &db, size_t) const {
  const auto it = db.find(pred_name);
  if (it == db.end())
    return {event_table()};
  event_table tab;
  for (const auto &ev : it->second) {
    auto new_row = match(ev);
    if (new_row)
      tab.add_row(std::move(*new_row));
  }
  vector<event_table> res(1, event_table());
  res[0] = std::move(tab);
  return res;
}

vector<event_table> MRel::eval(const database &, size_t) const { return {tab}; }

vector<event_table> MNeg::eval(const database &db, size_t ts) const {
  auto rec_tabs = state->eval(db, ts);
  vector<event_table> res;
  res.reserve(rec_tabs.size());
  std::transform(rec_tabs.cbegin(), rec_tabs.cend(), std::back_inserter(res),
                 [](const auto &tab) {
                   if (tab.is_empty())
                     return event_table::unit_table();
                   else
                     return event_table::empty_table();
                 });
  return res;
}

vector<event_table> MExists::eval(const database &db, size_t ts) const {
  auto rec_tabs = state->eval(db, ts);
  std::for_each(rec_tabs.begin(), rec_tabs.end(),
                [this](auto &tab) { tab.drop_col(idx_of_bound_var); });
  return rec_tabs;
}

vector<event_table> MOr::eval(const database &db, size_t ts) {
  auto l_rec_tabs = l_state->eval(db, ts), r_rec_tabs = r_state->eval(db, ts);
  auto reduction_fn = [this](const event_table &tab1, const event_table &tab2) {
    return tab1.t_union(tab2, r_layout_permutation);
  };
  return buf.update_and_reduce(l_rec_tabs, r_rec_tabs, reduction_fn);
}

}// namespace monitor::detail
