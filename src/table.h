#ifndef TABLE_H
#define TABLE_H

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <algorithm>
#include <cstddef>
#include <exception>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <hash_cache.h>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <util.h>
#include <utility>
#include <vector>

// TODO: implement efficient swap operation

namespace detail {
template<typename T>
class table;
}
/*
template<typename T>
struct [[maybe_unused]] fmt::formatter<detail::table<T>>;*/

namespace detail {
using absl::flat_hash_map;
using absl::flat_hash_set;
using std::pair;
using std::size_t;
using std::tuple;
using std::vector;

// Map from idx to variable name
using table_layout = vector<size_t>;

struct join_info {
  vector<size_t> comm_idx1, comm_idx2, keep_idx2;
  table_layout result_layout;
};

struct anti_join_info {
  vector<size_t> comm_idx1, comm_idx2;
  table_layout result_layout;
};

join_info get_join_info(const table_layout &l1, const table_layout &l2);
anti_join_info get_anti_join_info(const table_layout &l1,
                                  const table_layout &l2);
vector<size_t> find_permutation(const table_layout &l1, const table_layout &l2);
vector<size_t> id_permutation(size_t n);

template<typename T>
class table {
  // friend struct fmt::formatter<table<T>>;

public:
  using raw_row = vector<T>;
  using row_t = common::hash_cached<raw_row>;
  using data_t = flat_hash_set<row_t>;
  using data_t_ptr = typename data_t::const_pointer;
  using iterator = typename data_t::iterator;
  using const_iterator = typename data_t::const_iterator;
  using join_hash_map = flat_hash_map<row_t, vector<data_t_ptr>>;
  using join_hash_set = flat_hash_set<row_t>;

  static row_t filter_row(const vector<size_t> &keep_idxs, const row_t &row) {
    vector<T> filtered_row_inner;
    filtered_row_inner.reserve(keep_idxs.size());
    const auto &raw_row = row.key();
    std::transform(keep_idxs.cbegin(), keep_idxs.cend(),
                   std::back_inserter(filtered_row_inner),
                   [&raw_row](size_t idx) { return raw_row[idx]; });
    return std::move(filtered_row_inner);
  }

  static join_hash_map compute_join_hash_map(const table<T> &tab,
                                             const vector<size_t> &idxs) {
    join_hash_map res;
    res.reserve(tab.data_.size());
    for (const auto &row : tab.data_) {
      auto col_vals = filter_row(idxs, row);
      res[col_vals].push_back(&row);
    }
    return res;
  }

  static join_hash_set compute_join_hash_set(const table<T> &tab,
                                             const vector<size_t> &idxs) {
    join_hash_set res;
    res.reserve(tab.data_.size());
    for (const auto &row : tab.data_) {
      auto col_vals = filter_row(idxs, row);
      res.insert(std::move(col_vals));
    }
    return res;
  }

  static join_hash_set hash_all_destructive(table<T> &tab) {
    join_hash_set res;
    res.reserve(tab.data_.size());
    for (auto &row : tab.data_) {
      res.insert(std::move(row));
    }
    return res;
  }

  iterator begin() { return data_.begin(); }
  iterator end() { return data_.end(); }
  const_iterator begin() const { return data_.begin(); }
  const_iterator end() const { return data_.end(); }
  const_iterator cbegin() const { return data_.cbegin(); }
  const_iterator cend() const { return data_.cend(); }

  table() = default;

  explicit table(size_t n_cols) { ncols_ = n_cols; }

  explicit table(size_t n_cols, vector<vector<T>> data) {
    ncols_ = n_cols;
#ifndef NDEBUG
    bool table_match =
      std::all_of(data.cbegin(), data.cend(),
                  [n_cols](const auto &row) { return row.size() == n_cols; });
    if (!table_match)
      fmt::print(FMT_STRING("data row with wrong length"), data);
#endif
    data_.template insert(std::make_move_iterator(data.begin()),
                          std::make_move_iterator(data.end()));
  }

  [[nodiscard]] static table<T> empty_table() { return table(0, {}); };

  [[nodiscard]] static table<T> unit_table() { return table(0, {{}}); }

  [[nodiscard]] static table<T> singleton_table(T value) {
    return table(1, {{value}});
  }
  [[nodiscard]] bool empty() const { return data_.empty(); }

  [[nodiscard]] size_t tab_size() const { return data_.size(); }

  bool equal_to(const table<T> &other,
                const vector<size_t> &other_permutation) const {
    assert((ncols_ == other.ncols_) && (ncols_ == other_permutation.size()));
    if (tab_size() != other.tab_size())
      return false;
    for (const auto &row : other.data_) {
      auto permuted_row = filter_row(other_permutation, row);
      if (!data_.contains(permuted_row))
        return false;
    }
    return true;
  }

  bool contains(const typename data_t::key_type &row) const {
    return data_.contains(row);
  }

  void reserve(size_t n) { data_.reserve(n); }

  void add_row(const row_t &row) {
    assert(row.key().size() == ncols_);
    data_.insert(row);
  }

  void add_row(row_t &&row) {
    assert(row.key().size() == ncols_);
    data_.insert(std::move(row));
  }

  void add_row(const raw_row &row) {
    assert(row.size() == ncols_);
    data_.insert(row_t(row));
  }

  void add_row(raw_row &&row) {
    assert(row.size() == ncols_);
    data_.insert(row_t(std::move(row)));
  }

  void drop_col(size_t idx) {
    size_t n = ncols_;
    --ncols_;
    assert(idx >= 0 && idx < n);
    data_t tmp_cp = data_;
    data_.clear();
    data_.reserve(tmp_cp.size());
    for (const auto &hashed_row : tmp_cp) {
      const auto &row = hashed_row.key();
      vector<T> tmp_vec;
      tmp_vec.reserve(n - 1);
      for (size_t i = 0; i < n; ++i) {
        if (i == idx)
          continue;
        tmp_vec.push_back(std::move(row[i]));
      }
      data_.insert(std::move(tmp_vec));
    }
  }

  vector<vector<T>> make_verdicts(const vector<size_t> &permutation) {
    assert(permutation.size() == ncols_);
    vector<vector<T>> verdicts;
    verdicts.reserve(tab_size());
    for (const auto &row : data_) {
      assert(row.key().size() == ncols_);
      auto filtered_row = filter_row(permutation, row);
      verdicts.push_back(filtered_row.move_key_out());
    }
    return verdicts;
  }

  template<typename F>
  table<T> filter(F f) const {
    table<T> res(ncols_);
    std::copy_if(data_.cbegin(), data_.cend(),
                 std::inserter(res.data_, res.data_.end()), f);
    return res;
  }

  table<T> natural_join(const table<T> &tab, const join_info &info) const {
    auto hash_map = compute_join_hash_map(tab, info.comm_idx2);
    table<T> new_tab(info.result_layout.size());
    for (const auto &row : this->data_) {
      auto col_vals = filter_row(info.comm_idx1, row);
      const auto it = hash_map.find(col_vals);
      if (it == hash_map.end())
        continue;
      for (const auto &row2_ptr : it->second) {
        vector<T> new_row = row.key();
        new_row.reserve(new_row.size() + info.keep_idx2.size());
        auto snd_part = filter_row(info.keep_idx2, *row2_ptr).move_key_out();
        new_row.insert(new_row.cend(), snd_part.cbegin(), snd_part.cend());
        new_tab.data_.insert(std::move(new_row));
      }
    }
    return new_tab;
  }

  table<T> anti_join(const table<T> &tab, const anti_join_info &info) const {
    auto hash_set = compute_join_hash_set(tab, info.comm_idx2);
    table<T> new_tab(info.result_layout.size());
    for (const auto &row : data_) {
      auto filtered_row = filter_row(info.comm_idx1, row);
      if (!hash_set.contains(filtered_row))
        new_tab.data_.insert(row);
    }
    return new_tab;
  }


  void anti_join_in_place(const table<T> &tab, const anti_join_info &info) {
    auto hash_set = compute_join_hash_set(tab, info.comm_idx2);
    absl::erase_if(data_, [&info, &hash_set](const auto &row) {
      auto filtered_row = filter_row(info.comm_idx1, row);
      return hash_set.contains(filtered_row);
    });
  }

  table<T> t_union(const table<T> &tab,
                   const vector<size_t> &other_permutation) const {
    return t_union_impl(*this, tab, other_permutation);
  }

  void t_union_in_place(const table<T> &tab,
                        const vector<size_t> &other_permutation) {
    t_union_impl(*this, tab, other_permutation);
  }

private:
  size_t ncols_{};
  data_t data_;

  template<typename TAB1>
  static auto t_union_impl(TAB1 &tab1, const table<T> &tab2,
                           const vector<size_t> &other_permutation) {
    static_assert(std::is_same_v<std::remove_const_t<TAB1>, table<T>>,
                  "tables must have same type");
    if constexpr (std::is_const_v<TAB1>) {
      table<T> new_tab(tab1.ncols_);
      new_tab.data_ = tab1.data_;
      for (const auto &row : tab2.data_) {
        auto permuted_row = filter_row(other_permutation, row);
        new_tab.add_row(std::move(permuted_row));
      }
      return new_tab;
    } else {
      for (const auto &row : tab2.data_) {
        auto permuted_row = filter_row(other_permutation, row);
        tab1.add_row(std::move(permuted_row));
      }
    }
  }
};
}// namespace detail

using detail::anti_join_info;
using detail::find_permutation;
using detail::get_anti_join_info;
using detail::get_join_info;
using detail::id_permutation;
using detail::join_info;
using detail::table;
using detail::table_layout;


/*template<typename T>
struct [[maybe_unused]] fmt::formatter<table<T>> {
  constexpr auto parse [[maybe_unused]] (format_parse_context &ctx)
  -> decltype(auto) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}')
      throw format_error("invalid format - only empty format strings are "
                         "accepted for tables");
    return it;
  }

  template<typename FormatContext>
  auto format [[maybe_unused]] (const table<T> &tab, FormatContext &ctx)
  -> decltype(auto) {
    auto new_out = format_to(ctx.out(), "Table with {} columns\n", tab.ncols_);
    new_out = format_to(new_out, "Table Data:");
    if (tab.empty())
      new_out = format_to(new_out, "\nnone");
    else {
      for (const auto &row : tab.data_)
        new_out = format_to(new_out, "\n{}", row);
    }
    return new_out;
  }
};*/

#endif