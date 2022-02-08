#ifndef CPPMON_TEMPORAL_AGGREGATION_IMPL_H
#define CPPMON_TEMPORAL_AGGREGATION_IMPL_H

#include <absl/container/btree_set.h>
#include <absl/container/flat_hash_map.h>
#include <aggregation_impl.h>
#include <boost/variant2/variant.hpp>
#include <event_data.h>
#include <monitor_types.h>
#include <utility>

namespace monitor::detail::agg_temporal {

template<typename Compare>
class min_max_group {
public:
  min_max_group(const common::event_data &first_event) {
    group_state_.insert(first_event);
  }

  template<typename H>
  friend H AbslHashValue(H h, const min_max_group<Compare> &arg) {
    return H::combine(std::move(h), arg.group_state_);
  }

  void add_event(const common::event_data &val) { group_state_.insert(val); }

  void remove_event(const common::event_data &val) {
    auto it = group_state_.find(val);
    assert(it != group_state_.end());
    group_state_.erase(it);
  }

  common::event_data finalize_group() const {
    assert(!group_state_.empty());
    return *group_state_.begin();
  }

private:
  absl::btree_multiset<common::event_data, Compare> group_state_;
};

class count_group;

class sum_group : public agg_base::sum_group {
public:
  sum_group(const common::event_data &first_event)
      : agg_base::sum_group(first_event) {}

  void remove_event(const common::event_data &val) { res_ = res_ - val; }
};

class avg_group : public agg_base::avg_group {
public:
  avg_group(const common::event_data &first_value)
      : agg_base::avg_group(first_value) {}

  void remove_event(const common::event_data &val) {
    assert(counter_ > 0);
    sum_ = sum_ - val.to_double();
    counter_--;
  }
};

using max_group = min_max_group<std::greater<>>;
using min_group = min_max_group<std::less<>>;

template<typename GroupType>
class counted_group {
public:
  counted_group(const common::event_data &first_event)
      : nested_group_(GroupType(first_event)), counter_(1) {}

  bool empty() const { return counter_ == 0; }

  template<typename H>
  friend H AbslHashValue(H h, const counted_group<GroupType> &arg) {
    return H::combine(std::move(h), arg.nested_group_, arg.counter_);
  }

  void add_event(const common::event_data &val) {
    nested_group_.add_event(val);
    counter_++;
  }

  void remove_event(const common::event_data &val) {
    assert(counter_ > 0);
    nested_group_.remove_event(val);
    counter_--;
  }

  common::event_data finalize_group() const {
    assert(!empty());
    return nested_group_.finalize_group();
  }

private:
  GroupType nested_group_;
  size_t counter_;
};

template<>
class counted_group<count_group> {
public:
  counted_group(const common::event_data &) : counter_(1) {}

  bool empty() const { return counter_ == 0; }

  template<typename H>
  friend H AbslHashValue(H h, const counted_group<count_group> &arg) {
    return H::combine(std::move(h), arg.counter_);
  }

  void add_event(const common::event_data &) { counter_++; }

  void remove_event(const common::event_data &) {
    assert(counter_ > 0);
    counter_--;
  }

  common::event_data finalize_group() const {
    return common::event_data::Int(static_cast<int64_t>(counter_));
  }

private:
  size_t counter_;
};

template<typename GroupType>
class grouped_state : public agg_base::grouped_state<counted_group<GroupType>> {
public:
  grouped_state() : agg_base::grouped_state<counted_group<GroupType>>() {}

  void remove_result(const event &group, const common::event_data &val) {
    auto it = this->groups_.find(group);
    assert(it != this->groups_.end());
    it->second.remove_event(val);
    if (it->second.empty())
      this->groups_.erase(it);
  }
};

class temporal_aggregation_impl {
public:
  temporal_aggregation_impl(const table_layout &phi_layout,
                            const fo::Term &agg_term,
                            common::event_data default_val, fo::agg_type ty,
                            size_t res_var, size_t num_bound_vars);
  void add_result(const event &e);
  void remove_result(const event &e);
  opt_table finalize_table();
  table_layout get_layout() const;

private:
  using state_t =
    boost::variant2::variant<grouped_state<count_group>,
                             grouped_state<avg_group>, grouped_state<max_group>,
                             grouped_state<min_group>,
                             grouped_state<sum_group>>;
  common::event_data default_val_;
  std::vector<size_t> group_var_idxs_;
  size_t nfvs_;
  size_t term_var_idx_;
  table_layout res_layout_;
  state_t state_;
  bool all_bound_;
};

class no_temporal_aggregation {};

}// namespace monitor::detail::agg_temporal


#endif// CPPMON_TEMPORAL_AGGREGATION_IMPL_H
