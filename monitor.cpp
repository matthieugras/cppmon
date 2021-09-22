#include <monitor.h>

namespace monitor::detail {
// Monitor methods
Monitor::Monitor(const Formula &formula) : curr_tp(0) {
  // TODO: verify that the first TP is 0
  auto [state_tmp, layout_tmp] = MState::make_formula_state(formula, 0);
  state = MState(std::move(state_tmp));
  res_layout = std::move(layout_tmp);
}

satisfactions_t Monitor::step(const database &db, size_t ts) {
  size_t n_tps = 0;
  if (!db.empty()) {
    n_tps = db.cbegin()->second.size();
  }
  auto sats = state->eval(db, n_tps, ts);
  satisfactions_t transformed_sats;
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

vector<table_type> MState::eval(const database &db, size_t n_tps, size_t ts) {
  auto visitor = [&db, n_tps, ts](auto &&arg) -> vector<table_type> {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, MPred>) {
      return arg.eval(db, n_tps, ts);
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
      return {MRel{table_type::unit_table()}, {}};
    else
      return {MRel{table_type::empty_table()}, {}};
  }
  assert(l.is_var() && rcst || lcst && r.is_var());
  if (l.is_var())
    std::swap(lcst, rcst);
  return {MRel{table_type::singleton_table(0, *lcst)}, {0}};
}

pair<MState::val_type, table_layout>
MState::make_and_state(const fo::Formula::and_t &arg, size_t n_bound_vars) {
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
        val_type rec_st = make_formula_state(*arg.phi, n_bound_vars).first;
        return {MNeg{uniq(MState(std::move(rec_st)))}, {}};
      } else {
        return {MRel{table_type::empty_table()}, {}};
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
      // return MOr{uniq(MState(*arg.phil)), uniq(MState(*arg.phir)), MBuf2()};
    } else if constexpr (is_same_v<T, fo::Formula::and_t>) {
      throw not_implemented_error();
      // return make_and_state(arg, n_bound_vars);
    } else if constexpr (is_same_v<T, fo::Formula::exists_t>) {
      throw not_implemented_error();
      /*auto new_state =
        uniq(MState(make_formula_state(*arg.phi, n_bound_vars + 1)));
      return MExists{n_bound_vars, std::move(new_state)};*/
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

vector<table_type> MPred::eval(const database &db, size_t n_tps, size_t) const {
  const auto it = db.find(pred_name);
  if (it == db.end())
    return {n_tps, table_type()};
  vector<table_type> res;
  res.reserve(n_tps);
  for (const auto &ev_set : it->second) {
    table_type tab;
    for (const auto &ev : ev_set) {
      auto new_row = match(ev);
      if (new_row) {
        tab.add_row(std::move(*new_row));
      }
    }
    res.push_back(std::move(tab));
  }
  return res;
}

vector<table_type> MRel::eval(const database &, size_t, size_t) const {
  return {std::move(tab)};
}

vector<table_type> MNeg::eval(const database &db, size_t n_tps, size_t ts) const {
  auto rec_tabs = state->eval(db, n_tps, ts);
  vector<table_type> res;
  res.reserve(rec_tabs.size());
  std::transform(rec_tabs.cbegin(), rec_tabs.cend(), std::back_inserter(res),
                 [](const auto &tab) {
                   if (tab.is_empty())
                     return table_type::unit_table();
                   else
                     return table_type::empty_table();
                 });
  return res;
}

}// namespace monitor::detail
