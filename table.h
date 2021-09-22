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
using std::size_t;
using std::vector;

using table_layout = vector<size_t>;

table_layout get_join_layout(const table_layout &l1, const table_layout &l2);

template<typename T>
class table {
  friend struct fmt::formatter<table<T>>;

public:
  table() = default;
  explicit table(vector<size_t> idx_to_var) {
    size_t n = idx_to_var.size();
    m_n_cols = n;
    for (size_t i = 0; i < n; ++i) {
      this->var_to_idx[idx_to_var[i]] = i;
    }
    this->idx_to_var = std::move(idx_to_var);
  }
  explicit table(vector<size_t> idx_to_var, vector<vector<T>> data)
      : table(std::move(idx_to_var)) {
#ifndef NDEBUG
    size_t n = m_n_cols;
    bool table_match =
      std::all_of(data.cbegin(), data.cend(),
                  [n](const auto &row) { return row.size() == n; });
    if (!table_match)
      fmt::print(FMT_STRING("Table missmatch {} {}"), idx_to_var, data);
#endif
    m_tab_impl.template insert(std::make_move_iterator(data.begin()),
                               std::make_move_iterator(data.end()));
  }
  [[nodiscard]] static table<T> empty_table() { return table({}, {}); };
  [[nodiscard]] static table<T> unit_table() {
    // TODO: change API
    return table({}, {{}});
  }
  [[nodiscard]] static table<T> singleton_table(size_t var_name, T value) {
    return table({var_name}, {{value}});
  }
  [[nodiscard]] bool is_empty() const {
    return m_tab_impl.empty();
  }
  [[nodiscard]] const vector<size_t> &var_names() const {
    return this->idx_to_var;
  }
  bool operator==(const table<T> &other) const {
    if (this->m_n_cols != other.m_n_cols ||
        this->m_tab_impl.size() != other.m_tab_impl.size())
      return false;

    vector<size_t> permutation;
    bool ret = this->find_permutation(other, permutation);
    if (!ret)
      return false;
    for (const auto &row : other.m_tab_impl) {
      auto permuted_row = filter_row(permutation, row);
      if (!this->m_tab_impl.contains(permuted_row))
        return false;
    }
    return true;
  }
  bool operator!=(const table<T> &other) const { return !(*this == other); }
  size_t tab_size() { return this->m_tab_impl.size(); }
  void add_row(vector<T> row) {
    assert(row.size() == this->m_n_cols);
    m_tab_impl.insert(std::move(row));
  }

  table<T> natural_join(const table<T> &tab) const {
    vector<ptrdiff_t> idx_to_var2_filtered;
    vector<size_t> comm_idx1, comm_idx2;
    size_t n1 = this->m_n_cols, n2 = tab.m_n_cols;
    common_col_idxs(*this, tab, idx_to_var2_filtered, comm_idx1, comm_idx2);
    JoinHashTbl hash_map = tab.get_join_hash_table(comm_idx2);
    table<T> new_tab;
    new_tab.m_n_cols = n1 + n2 - comm_idx1.size();
    new_tab.var_to_idx = this->var_to_idx;
    new_tab.idx_to_var = this->idx_to_var;
    new_tab.idx_to_var.reserve(n1 + n2);
    for (size_t i = 0, count = 0; i < n2; ++i) {
      size_t idx = n1 + count;
      ptrdiff_t var = idx_to_var2_filtered[i];
      if (var != -1) {
        new_tab.idx_to_var.push_back(var);
        new_tab.var_to_idx[var] = idx;
        count++;
      }
    }
    for (const auto &row : this->m_tab_impl) {
      auto col_vals = filter_row(comm_idx1, row);
      const auto it = hash_map.find(col_vals);
      if (it == hash_map.end())
        continue;
      for (const auto &row2_ptr : it->second) {
        vector<T> new_row = row;
        for (size_t i = 0; i < n2; ++i) {
          if (idx_to_var2_filtered[i] == -1)
            continue;
          else
            new_row.push_back((*row2_ptr)[i]);
        }
        new_tab.m_tab_impl.insert(std::move(new_row));
      }
    }
    return new_tab;
  }
  table<T> anti_join(const table<T> &tab) const {
    vector<ptrdiff_t> idx_to_var2_filtered;
    vector<size_t> comm_idx1, comm_idx2;
    common_col_idxs(*this, tab, idx_to_var2_filtered, comm_idx1, comm_idx2);
    JoinHashTbl hash_map = tab.get_join_hash_table(comm_idx2);
    table<T> new_tab;
    new_tab.idx_to_var = this->idx_to_var;
    new_tab.m_n_cols = this->m_n_cols;
    new_tab.var_to_idx = this->var_to_idx;
    for (const auto &row : this->m_tab_impl) {
      const auto col_vals = filter_row(comm_idx1, row);
      if (!hash_map.contains(col_vals)) {
        new_tab.m_tab_impl.insert(row);
      }
    }
    return new_tab;
  }

  table<T> t_union(const table<T> &tab) const {
    return t_union_impl(*this, tab);
  }

  void t_union_in_place(const table<T> &tab) { t_union_impl(*this, tab); }

private:
  //table() = default;

  using TblImplType = flat_hash_set<vector<T>>;
  using TblImplPtr = typename TblImplType::const_pointer;
  using Var2IdxMap = flat_hash_map<size_t, size_t>;
  using JoinHashTbl = flat_hash_map<vector<T>, vector<TblImplPtr>>;

  size_t m_n_cols;
  TblImplType m_tab_impl;
  vector<size_t> idx_to_var;
  Var2IdxMap var_to_idx;

  template<typename TAB1>
  static auto t_union_impl(TAB1 &tab1, const table<T> &tab2) {
    static_assert(std::is_same_v<std::remove_const_t<TAB1>, table<T>>,
                  "tables must have same type");
    vector<size_t> permutation;
    bool ret = tab1.find_permutation(tab2, permutation);
#ifndef NDEBUG
    if (!ret)
      throw std::invalid_argument("table layouts are incompatible");
#endif
    if constexpr (std::is_const_v<TAB1>) {
      table<T> new_tab;
      new_tab.m_n_cols = tab1.m_n_cols;
      new_tab.m_tab_impl = tab1.m_tab_impl;
      new_tab.idx_to_var = tab1.idx_to_var;
      new_tab.var_to_idx = tab1.var_to_idx;
      for (const auto &row : tab2.m_tab_impl) {
        auto permuted_row = filter_row(permutation, row);
        new_tab.m_tab_impl.insert(std::move(permuted_row));
      }
      return new_tab;
    } else {
      for (const auto &row : tab2.m_tab_impl) {
        auto permuted_row = filter_row(permutation, row);
        tab1.m_tab_impl.insert(std::move(permuted_row));
      }
    }
  }

  bool find_permutation(const table<T> &other,
                        vector<size_t> &permutation) const {
    size_t n = this->m_n_cols;
    if (this->m_n_cols != other.m_n_cols)
      return false;
    permutation.reserve(n);
    for (const auto &var : this->idx_to_var) {
      const auto it_idx2 = other.var_to_idx.find(var);
      if (it_idx2 == other.var_to_idx.end())
        return false;
      permutation.push_back(it_idx2->second);
    }
    return true;
  }

  JoinHashTbl get_join_hash_table(const vector<size_t> &idxs) const {
    JoinHashTbl hash_map;
    size_t i = 0;
    for (const auto &row : this->m_tab_impl) {
      auto col_vals = filter_row(idxs, row);
      hash_map[col_vals].push_back(&row);
      ++i;
    }
    return hash_map;
  }

  static void common_col_idxs(const table<T> &tab1, const table<T> &tab2,
                              vector<ptrdiff_t> &idx_to_var2_filtered,
                              vector<size_t> &comm_idx1,
                              vector<size_t> &comm_idx2) {
    size_t n1 = tab1.m_n_cols, n2 = tab2.m_n_cols;
    size_t max_common_cols = std::min(n1, n2);
    idx_to_var2_filtered.reserve(n2);
    comm_idx1.reserve(max_common_cols);
    comm_idx2.reserve(max_common_cols);
    for (size_t i = 0; i < n2; ++i) {
      size_t var = tab2.idx_to_var[i];
      auto it = tab1.var_to_idx.find(var);
      if (it == tab1.var_to_idx.end()) {
        idx_to_var2_filtered.push_back(var);
      } else {
        comm_idx1.push_back(it->second);
        comm_idx2.push_back(i);
        idx_to_var2_filtered.push_back(-1);
      }
    }
  }
  static vector<T> filter_row(const vector<size_t> &keep_idxs,
                              const vector<T> &row) {
    vector<T> filtered_row;
    filtered_row.reserve(keep_idxs.size());
    std::transform(keep_idxs.cbegin(), keep_idxs.cend(),
                   std::back_inserter(filtered_row),
                   [&row](size_t idx) { return static_cast<T>(row[idx]); });
    return filtered_row;
  }
};
}// namespace tbl_impl

using detail::table;
using detail::table_layout;
using detail::get_join_layout;

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
    auto new_out =
      format_to(ctx.out(), "Table with {} columns\n", tab.m_n_cols);
    new_out = format_to(new_out, "{}\n", tab.idx_to_var);
    new_out = format_to(new_out, "Table Data:");
    if (tab.m_tab_impl.empty())
      new_out = format_to(new_out, "\nnone");
    else {
      for (const auto &row : tab.m_tab_impl)
        new_out = format_to(new_out, "\n{}", row);
    }
    return new_out;
  }
};

#endif