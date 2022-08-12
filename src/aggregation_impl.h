#ifndef CPPMON_AGGREGATION_IMPL_H
#define CPPMON_AGGREGATION_IMPL_H

#include <absl/container/flat_hash_map.h>
#include <algorithm>
#include <boost/variant2/variant.hpp>
#include <fmt/core.h>
#include <formula.h>
#include <monitor_types.h>
#include <table.h>
#include <utility>
#include <vector>

namespace monitor::detail::agg_base {
template<typename GroupType>
class grouped_state {
public:
  grouped_state() = default;

  void add_result(const event &group, const common::event_data &val) {
    auto it = groups_.template find(group);
    if (it == groups_.end()) {
      groups_.emplace(group, GroupType(val));
    } else {
      it->second.add_event(val);
    }
  }

  opt_table finalize_table(size_t nfvs, bool clear_groups = true) {
    event_table res(nfvs);
    for (auto &[group, group_state] : groups_) {
      auto result_value = group_state.finalize_group();
      event group_cp;
      group_cp.reserve(nfvs);
      group_cp.insert(group_cp.end(), group.cbegin(), group.cend());
      group_cp.push_back(std::move(result_value));
      res.add_row(std::move(group_cp));
    }
    if (clear_groups)
      groups_.clear();
    return res.empty() ? opt_table() : std::move(res);
  }

protected:
  absl::flat_hash_map<event, GroupType> groups_;
};

class simple_combiner {
public:
  simple_combiner(const common::event_data &first_event) : res_(first_event) {}

  template<typename H>
  friend H AbslHashValue(H h, const simple_combiner &arg) {
    return H::combine(std::move(h), arg.res_);
  }

  common::event_data finalize_group() const { return res_; }

protected:
  common::event_data res_;
};

class max_group : public simple_combiner {
public:
  max_group(const common::event_data &first_event)
      : simple_combiner(first_event) {}

  void add_event(const common::event_data &val) {
    if (val > res_)
      res_ = val;
  }
};

class min_group : public simple_combiner {
public:
  min_group(const common::event_data &first_event)
      : simple_combiner(first_event) {}

  void add_event(const common::event_data &val) {
    if (val < res_)
      res_ = val;
  }
};

class sum_group : public simple_combiner {
public:
  sum_group(const common::event_data &first_event)
      : simple_combiner(first_event) {}

  void add_event(const common::event_data &val) { res_ = res_ + val; }
};

class count_group {
public:
  count_group(const common::event_data &) : counter_(1) {}

  template<typename H>
  friend H AbslHashValue(H h, const count_group &arg) {
    return H::combine(std::move(h), arg.counter_);
  }

  void add_event(const common::event_data &) { counter_++; }

  common::event_data finalize_group() const {
    return common::event_data::Int(static_cast<int64_t>(counter_));
  }

protected:
  size_t counter_;
};

class avg_group {
public:
  using val_type = std::pair<common::event_data, size_t>;
  avg_group(const common::event_data &first_event)
      : sum_(first_event.to_double()), counter_(1) {}

  template<typename H>
  friend H AbslHashValue(H h, const avg_group &arg) {
    return H::combine(std::move(h), arg.sum_, arg.counter_);
  }

  void add_event(const common::event_data &val) {
    sum_ += val.to_double();
    counter_++;
  }

  common::event_data finalize_group() const {
    return common::event_data::Float(sum_ / static_cast<double>(counter_));
  }

protected:
  double sum_;
  size_t counter_;
};

class med_group {
public:
  med_group(const common::event_data &first_event)
      : values_{first_event} {}

  template<typename H>
  friend H AbslHashValue(H h, const med_group &arg) {
    return H::combine(std::move(h), arg.values_);
  }

  void add_event(const common::event_data &val) {
    values_.emplace_back(val);
  }

  common::event_data finalize_group() {
    std::sort(values_.begin(), values_.end(), common::compat_less());
    size_t n = values_.size();
    if (n % 2 == 0) {
      double med =
        (values_.at(n/2).to_double() + values_.at(n/2 - 1).to_double()) / 2.0;
      return common::event_data::Float(med);
    } else {
      return common::event_data::Float(values_.at(n/2).to_double());
    }
  }

protected:
  std::vector<common::event_data> values_;
};

class aggregation_impl {
public:
  aggregation_impl(const table_layout &phi_layout, const fo::Term &agg_term,
                   common::event_data default_val, fo::agg_type ty,
                   size_t res_var, size_t num_bound_vars);
  opt_table eval(opt_table &tab);
  table_layout get_layout() const;

private:
  using state_t =
    boost::variant2::variant<grouped_state<count_group>,
                             grouped_state<avg_group>, grouped_state<max_group>,
                             grouped_state<min_group>,
                             grouped_state<sum_group>,
                             grouped_state<med_group>>;
  common::event_data default_val_;
  std::vector<size_t> group_var_idxs_;
  size_t nfvs_;
  size_t term_var_idx_;
  table_layout res_layout_;
  state_t state_;
  bool all_bound_;
};

}// namespace monitor::detail::agg_base

#endif
