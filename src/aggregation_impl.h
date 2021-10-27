#ifndef CPPMON_AGGREGATION_IMPL_H
#define CPPMON_AGGREGATION_IMPL_H

#include <absl/container/flat_hash_map.h>
#include <boost/variant2/variant.hpp>
#include <fmt/core.h>
#include <formula.h>
#include <monitor_types.h>
#include <table.h>
#include <utility>
#include <vector>

namespace monitor::detail {

template<typename Combiner, typename BufType>
class grouped_state {
public:
  void add_result(event &group, common::event_data &val) {
    auto as_combiner = static_cast<Combiner *>(this);
    auto it = groups_.template find(group);
    if (it == groups_.end()) {
      // fmt::print("adding new element {}\n", val);
      groups_.template emplace(group, as_combiner->init_group(val));
    } else {
      // fmt::print("combining results {} and {}\n", it->second, val);
      as_combiner->combine(it->second, val);
    }
  }

  event_table finalize_table(size_t nfvs) {
    auto as_combiner = static_cast<Combiner *>(this);
    event_table res(nfvs);
    for (auto &[group, combiner_val] : groups_) {
      auto result_value = as_combiner->finalize_group(combiner_val);
      event group_cp;
      group_cp.reserve(nfvs);
      group_cp.insert(group_cp.end(), group.cbegin(), group.cend());
      group_cp.push_back(std::move(result_value));
      res.add_row(std::move(group_cp));
    }
    groups_.clear();
    return res;
  }

private:
  absl::flat_hash_map<event, BufType> groups_;
};

class simple_combiner {
public:
  using val_type = common::event_data;
  simple_combiner() {}
  common::event_data init_group(common::event_data &e) const { return e; }
  common::event_data finalize_group(common::event_data &e) const { return e; }
};

class max_combiner
    : public simple_combiner,
      public grouped_state<max_combiner, typename simple_combiner::val_type> {
public:
  max_combiner() {}

  void combine(common::event_data &a, common::event_data &b) const {
    if (b > a)
      std::swap(a, b);
  }
};

class min_combiner
    : public simple_combiner,
      public grouped_state<min_combiner, typename simple_combiner::val_type> {
public:
  min_combiner() {}

  void combine(common::event_data &a, common::event_data &b) const {
    if (b < a)
      std::swap(a, b);
  }
};

class sum_combiner
    : public simple_combiner,
      public grouped_state<sum_combiner, typename simple_combiner::val_type> {
public:
  sum_combiner() {}

  void combine(common::event_data &a, common::event_data &b) const {
    a = a + b;
  }
};

class count_combiner : public grouped_state<count_combiner, size_t> {
public:
  count_combiner() {}

  size_t init_group(common::event_data &) const { return 1; }

  common::event_data finalize_group(size_t counter) const {
    return common::event_data::Int(static_cast<int>(counter));
  }

  void combine(size_t &curr_count, common::event_data &) const { curr_count++; }
};

class avg_combiner
    : public grouped_state<avg_combiner,
                           std::pair<common::event_data, size_t>> {
public:
  using val_type = std::pair<common::event_data, size_t>;
  avg_combiner() {}

  val_type init_group(common::event_data &e) const { return std::pair(e, 1); }

  common::event_data finalize_group(val_type &res) const {
    return common::event_data::Float(res.first.to_double() /
                                     static_cast<double>(res.second));
  }

  void combine(val_type &a, common::event_data &b) const {
    a.first = a.first + b;
    a.second++;
  }
};

class aggregation_impl {
public:
  aggregation_impl(const table_layout &phi_layout, const fo::Term &agg_term,
                   common::event_data default_val, fo::agg_type ty,
                   size_t res_var, size_t num_bound_vars);
  event_table eval(event_table &tab);
  table_layout get_layout() const;

private:
  using combiner_t =
    boost::variant2::variant<count_combiner, avg_combiner, max_combiner,
                             min_combiner, sum_combiner>;
  common::event_data default_val_;
  std::vector<size_t> group_var_idxs_;
  size_t nfvs_;
  size_t term_var_idx_;
  table_layout res_layout_;
  combiner_t combiner_;
  bool all_bound_;
};

}// namespace monitor::detail

#endif
