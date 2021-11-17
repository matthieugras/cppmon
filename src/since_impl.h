#ifndef CPPMON_SINCE_IMPL_H
#define CPPMON_SINCE_IMPL_H

#include <absl/container/flat_hash_map.h>
#include <boost/container/devector.hpp>
#include <event_data.h>
#include <formula.h>
#include <monitor_types.h>
#include <table.h>
#include <temporal_aggregation_impl.h>
#include <vector>

namespace monitor::detail {
using tuple_buf = absl::flat_hash_map<std::vector<common::event_data>, size_t>;

template<typename SinceBase>
class shared_agg_base {
public:
  event_table produce_result() { return temporal_agg_.finalize_table(); }

protected:
  shared_agg_base(agg_temporal::temporal_aggregation_impl temporal_agg)
      : temporal_agg_(std::move(temporal_agg)) {}

  void tuple_in_update(const event &e, size_t ts) {
    auto &tuple_in = static_cast<SinceBase *>(this)->tuple_in;
    auto it = tuple_in.find(e);
    if (it == tuple_in.end()) {
      temporal_agg_.add_result(e);
      tuple_in.emplace(e, ts);
    } else {
      it->second = ts;
    }
  }

  void tuple_in_erase(tuple_buf::iterator it) {
    temporal_agg_.remove_result(it->first);
    static_cast<SinceBase *>(this)->tuple_in.erase(it);
  }

  template<typename Pred>
  void tuple_in_erase_if(Pred pred) {
    auto combined_pred = [&pred, this](const tuple_buf::value_type &e) {
      if (pred(e)) {
        temporal_agg_.remove_result(e.first);
        return true;
      }
      return false;
    };
    absl::erase_if(static_cast<SinceBase *>(this)->tuple_in, combined_pred);
  }

private:
  agg_temporal::temporal_aggregation_impl temporal_agg_;
};

template<typename SinceBase>
class shared_no_agg {
public:
  event_table produce_result() {
    SinceBase *base = static_cast<SinceBase *>(this);
    auto &tuple_in = base->tuple_in;
    size_t nfvs = base->nfvs;

    event_table tab(nfvs);
    tab.reserve(tuple_in.size());
    for (auto it = tuple_in.cbegin(); it != tuple_in.cend();) {
      auto nxt_it = it;
      nxt_it++;
      if (nxt_it != tuple_in.cend()) {
        __builtin_prefetch(nxt_it->first.data());
        __builtin_prefetch(nxt_it->first.data() + 4);
        auto nxt_nxt_it = nxt_it;
        nxt_nxt_it++;
        if (nxt_nxt_it != tuple_in.cend())
          __builtin_prefetch(nxt_nxt_it->first.data());
      }
      tab.add_row(it->first);
      it = nxt_it;
    }
    return tab;
  }

protected:
  shared_no_agg() {}

  void tuple_in_update(const event &e, size_t ts) {
    static_cast<SinceBase *>(this)->tuple_in.insert_or_assign(e, ts);
  }

  void tuple_in_erase(tuple_buf::iterator it) {
    static_cast<SinceBase *>(this)->tuple_in.erase(it);
  }

  template<typename Pred>
  void tuple_in_erase_if(Pred pred) {
    absl::erase_if(static_cast<SinceBase *>(this)->tuple_in, pred);
  }
};

template<typename AggBase>
class shared_base : public AggBase {
  friend AggBase;

protected:
  using table_buf = boost::container::devector<std::pair<size_t, event_table>>;

  template<typename... Args>
  shared_base(size_t nfvs, fo::Interval inter, Args &&...args)
      : AggBase(std::forward<Args>(args)...), nfvs(nfvs), inter(inter) {}

  void drop_tuple_from_all_data(const event &e) {
    auto all_dat_it = all_data_counted.find(e);
    assert(all_dat_it != all_data_counted.end() && all_dat_it->second > 0);
    if ((--all_dat_it->second) == 0) {
      tuple_since.erase(all_dat_it->first);
      all_data_counted.erase(all_dat_it);
    }
  }

  void drop_too_old(size_t ts) {
    for (; !data_in.empty(); data_in.pop_front()) {
      auto old_ts = data_in.front().first;
      assert(ts >= old_ts);
      if (inter.leq_upper(ts - old_ts))
        break;
      auto &tab = data_in.front();
      for (const auto &e : tab.second) {
        auto in_it = tuple_in.find(e);
        if (in_it != tuple_in.end() && in_it->second == tab.first)
          this->tuple_in_erase(in_it);
        drop_tuple_from_all_data(e);
      }
    }
    for (; !data_prev.empty() && inter.gt_upper(ts - data_prev.front().first);
         data_prev.pop_front()) {
      for (const auto &e : data_prev.front().second)
        drop_tuple_from_all_data(e);
    }
  }

  void add_new_ts(size_t ts) {
    drop_too_old(ts);
    for (; !data_prev.empty(); data_prev.pop_front()) {
      auto &latest = data_prev.front();
      size_t old_ts = latest.first;
      assert(old_ts <= ts);
      if (inter.lt_lower(ts - old_ts))
        break;
      for (const auto &e : latest.second) {
        auto since_it = tuple_since.find(e);
        if (since_it != tuple_since.end() && since_it->second <= old_ts)
          this->tuple_in_update(e, old_ts);
      }
      assert(data_in.empty() || old_ts >= data_in.back().first);
      data_in.push_back(std::move(latest));
    }
  }

  void add_new_table(event_table &&tab_r, size_t ts) {
    for (const auto &e : tab_r) {
      tuple_since.try_emplace(e, ts);
      all_data_counted[e]++;
    }
    if (inter.contains(0)) {
      for (const auto &e : tab_r)
        this->tuple_in_update(e, ts);
      assert(data_in.empty() || ts >= data_in.back().first);
      data_in.emplace_back(ts, std::move(tab_r));
    } else {
      assert(data_prev.empty() || ts >= data_prev.back().first);
      data_prev.emplace_back(ts, std::move(tab_r));
    }
  }

  size_t nfvs;
  fo::Interval inter;
  table_buf data_prev, data_in;
  tuple_buf tuple_in, tuple_since, all_data_counted;
};

template<typename SinceBase>
class since_base : public SinceBase {
public:
  event_table eval(event_table &tab_l, event_table &tab_r, size_t new_ts) {
    this->add_new_ts(new_ts);
    this->join(tab_l);
    this->add_new_table(std::move(tab_r), new_ts);
    return this->produce_result();
  }

protected:
  template<typename... Args>
  since_base(bool left_negated, std::vector<size_t> comm_idx_r, Args &&...args)
      : SinceBase(std::forward<Args>(args)...), left_negated(left_negated),
        comm_idx_r(std::move(comm_idx_r)) {}

  void join(event_table &tab_l) {
    auto hash_set = event_table::hash_all_destructive(tab_l);
    auto erase_cond = [this, &hash_set](const auto &tup) {
      if (left_negated)
        return hash_set.contains(filter_row(comm_idx_r, tup.first));
      else
        return !hash_set.contains(filter_row(comm_idx_r, tup.first));
    };
    absl::erase_if(this->tuple_since, erase_cond);
    this->tuple_in_erase_if(erase_cond);
  }

  bool left_negated;
  std::vector<size_t> comm_idx_r;
};

template<typename OnceBase>
class once_base : public OnceBase {
public:
  event_table eval(event_table &tab_r, size_t new_ts) {
    this->add_new_ts(new_ts);
    this->add_new_table(std::move(tab_r), new_ts);
    return this->produce_result();
  }

protected:
  template<typename... Args>
  once_base(Args &&...args) : OnceBase(std::forward<Args>(args)...) {}
};

class since_impl : public since_base<shared_base<shared_no_agg<since_impl>>> {
public:
  since_impl(bool left_negated, size_t nfvs, std::vector<size_t> comm_idx_r,
             fo::Interval inter);

  void print_state() {
    fmt::print("since_impl state: left_negated: {}, comm_idx_r: {}, data_prev: "
               "{}, data_in {}, tuple_in: {}, tuple_since: {}\n",
               left_negated, comm_idx_r, data_prev, data_in, tuple_in,
               tuple_since);
  }
};

class once_impl : public once_base<shared_base<shared_no_agg<once_impl>>> {
public:
  once_impl(size_t nfvs, fo::Interval inter);

  void print_state() {
    fmt::print("once_impl state: data_prev: {}, data_in {}, tuple_in: {}, "
               "tuple_since: {}\n",
               data_prev, data_in, tuple_in, tuple_since);
  }
};

class since_agg_impl
    : public since_base<shared_base<shared_agg_base<since_agg_impl>>> {
public:
  since_agg_impl(bool left_negated, size_t nfvs, std::vector<size_t> comm_idx_r,
                 fo::Interval inter,
                 agg_temporal::temporal_aggregation_impl temporal_agg);

  void print_state() {
    fmt::print("since_impl state: left_negated: {}, comm_idx_r: {}, data_prev: "
               "{}, data_in {}, tuple_in: {}, tuple_since: {}\n",
               left_negated, comm_idx_r, data_prev, data_in, tuple_in,
               tuple_since);
  }
};

class once_agg_impl
    : public once_base<shared_base<shared_agg_base<once_agg_impl>>> {
public:
  once_agg_impl(size_t nfvs, fo::Interval inter,
                agg_temporal::temporal_aggregation_impl temporal_agg);

  void print_state() {}
};


}// namespace monitor::detail


#endif// CPPMON_SINCE_IMPL_H
