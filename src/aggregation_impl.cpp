#include "aggregation_impl.h"
#include <limits>
#include <stdexcept>

namespace monitor::detail::agg_base {
aggregation_impl::aggregation_impl(const table_layout &phi_layout,
                                   const fo::Term &agg_term,
                                   common::event_data default_val,
                                   fo::agg_type ty, size_t res_var,
                                   size_t num_bound_vars)
    : default_val_(std::move(default_val)), nfvs_(0),
      term_var_idx_(std::numeric_limits<size_t>::max()),
      state_(grouped_state<min_group>()), all_bound_(true) {
  const size_t *trm_var_ptr = agg_term.get_if_var();
  if (!trm_var_ptr)
    throw std::runtime_error(
      "only simple var terms are supported (as in monpoly)");

  size_t trm_var = *trm_var_ptr;
  for (size_t idx = 0; idx < phi_layout.size(); ++idx) {
    size_t var = phi_layout[idx];
    assert(!(var == trm_var && var >= num_bound_vars));
    if (var == trm_var) {
      term_var_idx_ = idx;
    } else if (var >= num_bound_vars) {
      group_var_idxs_.push_back(idx);
      res_layout_.push_back(var - num_bound_vars);
      all_bound_ = false;
    }
  }
  res_layout_.push_back(res_var);
  nfvs_ = res_layout_.size();
  /*fmt::print("phi_layout: {}, res_var: {}, num_bound_vars: {}, nfvs: {}, "
             "all_vars_bound: {}, res_layout: {}\n",
             phi_layout, res_var, num_bound_vars, nfvs_, all_bound_,
             res_layout_);*/
  if (ty == fo::agg_type::SUM)
    state_ = grouped_state<sum_group>();
  else if (ty == fo::agg_type::MAX)
    state_ = grouped_state<max_group>();
  else if (ty == fo::agg_type::MIN)
    state_ = grouped_state<min_group>();
  else if (ty == fo::agg_type::CNT)
    state_ = grouped_state<count_group>();
  else if (ty == fo::agg_type::AVG)
    state_ = grouped_state<avg_group>();
  else
    throw std::runtime_error("unsupported operation");
}

event_table aggregation_impl::eval(event_table &tab) {
  if (tab.empty() && all_bound_)
    return event_table::singleton_table(default_val_);
  for (const auto &e : tab) {
    auto group = filter_row(group_var_idxs_, e);
    auto trm_var_val = e[term_var_idx_];
    boost::variant2::visit(
      [&group, &trm_var_val](auto &&arg) {
        // fmt::print("group is: {}, trm_var_val is: {}\n", group, trm_var_val);
        arg.add_result(group, trm_var_val);
      },
      state_);
  }
  event_table res_tab = boost::variant2::visit(
    [this](auto &&arg) { return arg.finalize_table(nfvs_); }, state_);
  return res_tab;
}

table_layout aggregation_impl::get_layout() const { return res_layout_; }
}// namespace monitor::detail::agg_base