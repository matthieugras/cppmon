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
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <util.h>
#include <utility>
#include <vector>

namespace detail {
template<typename T>
class table;
}
template<typename T>
struct [[maybe_unused]] fmt::formatter<detail::table<T>>;

namespace detail {
using absl::flat_hash_map;
using absl::flat_hash_set;
using std::pair;
using std::size_t;
using std::tuple;
using std::vector;

// Map from idx to variable name
using table_layout = vector<size_t>;

tuple<table_layout, vector<size_t>, vector<size_t>>
get_join_layout(const table_layout &l1, const table_layout &l2);
vector<size_t> find_permutation(const table_layout &l1, const table_layout &l2);
vector<size_t> id_permutation(size_t n);

template<typename T>
class table {
  friend struct fmt::formatter<table<T>>;

public:
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

  void add_row(const vector<T> &row) {
    assert(row.size() == ncols_);
    data_.insert(row);
  }

  void add_row(vector<T> &&row) {
    assert(row.size() == ncols_);
    data_.insert(std::move(row));
  }

  void drop_col(size_t idx) {
    size_t n = ncols_;
    assert(idx >= 0 && idx < n);
    data_t tmp_cp = data_;
    data_.clear();
    data_.reserve(tmp_cp.size());
    for (const auto &row : tmp_cp) {
      vector<T> tmp_vec;
      tmp_vec.reserve(n);
      for (size_t i = 0; i < n; ++i) {
        if (i == idx)
          continue;
        tmp_vec.push_back(std::move(row[i]));
      }
    }
  }

  template<typename F>
  table<T> filter(F f) const {
    table<T> res(ncols_);
    std::copy_if(data_.cbegin(), data_.cend(),
                 std::inserter(res.data_, res.data_.end()), f);
    return res;
  }

  table<T> natural_join(const table<T> &tab, const vector<size_t> &join_idx1,
                        const vector<size_t> &join_idx2) const {
    assert(std::is_sorted(join_idx1.cbegin(), join_idx1.cend()) &&
           std::is_sorted(join_idx2.cbegin(), join_idx2.cend()));
    assert(join_idx1.size() == join_idx2.size());
    size_t n1 = this->ncols_, n2 = tab.ncols_;
    auto hash_map = tab.template compute_join_hash<join_hash_table>(join_idx2);
    table<T> new_tab(n1 + n2 - join_idx1.size());
    auto keep_2_idx = complement_idx(join_idx2, n2);

    for (const auto &row : this->data_) {
      auto col_vals = filter_row(join_idx1, row);
      const auto it = hash_map.find(col_vals);
      if (it == hash_map.end())
        continue;
      for (const auto &row2_ptr : it->second) {
        vector<T> new_row = row;
        new_row.reserve(new_row.size() + keep_2_idx.size());
        auto snd_part = filter_row(keep_2_idx, *row2_ptr);
        new_row.insert(new_row.cend(), snd_part.cbegin(), snd_part.cend());
        new_tab.data_.insert(std::move(new_row));
      }
    }
    return new_tab;
  }

  table<T> anti_join(const table<T> &tab, const vector<size_t> &join_idx1,
                     const vector<size_t> &join_idx2) const {
    // TODO: fix & test this
    /*auto hash_map = tab.template compute_join_hash<TblImplType>(join_idx2);
    table<T> new_tab(m_n_cols);
    new_tab.idx_to_var = this->idx_to_var;
    new_tab.m_n_cols = this->m_n_cols;
    new_tab.var_to_idx = this->var_to_idx;
    for (const auto &row : this->m_tab_impl) {
      const auto col_vals = filter_row(join_idx1, row);
      if (!hash_map.contains(col_vals)) {
        new_tab.m_tab_impl.insert(row);
      }
    }
    return new_tab;*/
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
  // table() = default;

  using data_t = flat_hash_set<vector<T>>;
  using data_t_ptr = typename data_t::const_pointer;
  using join_hash_table = flat_hash_map<vector<T>, vector<data_t_ptr>>;

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

  template<typename R>
  R compute_join_hash(const vector<size_t> &idxs) const {
    R res{};
    res.reserve(data_.size());
    for (const auto &row : data_) {
      auto col_vals = filter_row(idxs, row);
      if constexpr (std::is_same_v<R, join_hash_table>) {
        res[col_vals].push_back(&row);
      } else if constexpr (std::is_same_v<R, data_t>) {
        res.insert(std::move(col_vals));
      } else {
        static_assert(always_false_v<R>, "can only return set or hashmap");
      }
    }
    return res;
  }

  static vector<T> filter_row(const vector<size_t> &keep_idxs,
                              const vector<T> &row) {
    vector<T> filtered_row;
    filtered_row.reserve(keep_idxs.size());
    std::transform(keep_idxs.cbegin(), keep_idxs.cend(),
                   std::back_inserter(filtered_row),
                   [&row](size_t idx) { return row[idx]; });
    return filtered_row;
  }

  static vector<size_t> complement_idx(const vector<size_t> &idx,
                                       size_t num_cols) {
    size_t n = idx.size();
    assert(num_cols >= n);
    vector<size_t> res;
    res.reserve(num_cols - n);
    for (size_t i = 0, curr_pos = 0; i < num_cols; ++i) {
      if (curr_pos >= n) {
        res.push_back(i);
      } else {
        size_t curr_idx = idx[curr_pos];
        assert(i <= curr_idx);
        if (i == curr_idx)
          curr_pos++;
        else
          res.push_back(i);
      }
    }
    return res;
  }
};
}// namespace detail

using detail::find_permutation;
using detail::get_join_layout;
using detail::id_permutation;
using detail::table;
using detail::table_layout;

template<typename T>
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
};

#endif