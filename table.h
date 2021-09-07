#ifndef TABLE_H
#define TABLE_H

#include <algorithm>
#include <boost/container_hash/extensions.hpp>
#include <boost/container_hash/hash.hpp>
#include <cstddef>
#include <exception>
#include <fmt/core.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <iterator>
#include <memory>
#include <tuple>
#include <utility>
#include <vector>
#include <absl/hash/hash.h>
#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>

namespace tbl_impl {
template <typename T> class table;
}
template <typename T> struct fmt::formatter<tbl_impl::table<T>>;
using tbl_impl::table;

namespace tbl_impl {
using std::size_t;
using absl::flat_hash_set;
using absl::flat_hash_map;
using std::vector;

template <typename T> class table {
  friend struct fmt::formatter<table<T>>;
  using TblImplType = flat_hash_set<vector<T>>;
  using TblImplPtr = typename TblImplType::const_pointer;
  using Var2IdxMap = flat_hash_map<size_t, size_t>;

public:
  explicit table(const vector<size_t> &idx_to_var) {
    size_t n = idx_to_var.size();
    this->m_n_cols = n;
    if (n == 0)
      return;
    for (size_t i = 0; i < n; ++i) {
      this->var_to_idx[idx_to_var[i]] = i;
    }
    this->idx_to_var = idx_to_var;
  }
  explicit table(const vector<size_t> &idx_to_var,
                 const vector<vector<T>> &data)
      : table(idx_to_var) {
    size_t n = idx_to_var.size();
    if (n == 0)
      return;
    for (const auto &row : data) {
      assert(row.size() == n);
      this->m_tab_impl.insert(row);
    }
  }
  const vector<size_t> &var_names() const { return this->idx_to_var; }
  bool operator==(const table<T> &other) const {
    if (this->m_n_cols != other.m_n_cols ||
        this->m_tab_impl.size() != other.m_tab_impl.size())
      return false;

    vector<size_t> permutation;
    bool ret = this->find_permutation(other, permutation);
    if (!ret)
      return false;
    for (const auto &row : other.m_tab_impl) {
      vector<T> permuted_row = permute_row(permutation, row);
      const auto it = this->m_tab_impl.find(permuted_row);
      if (it == this->m_tab_impl.end())
        return false;
    }
    return true;
  }
  bool operator!=(const table<T> &other) const { return !(*this == other); }
  size_t tab_size() { return this->m_tab_impl.size(); }
  void add_row(const vector<T> &row) {
    assert(row.size() == this->m_n_cols);
    this->m_tab_impl.insert(row);
  }
  table<T> natural_join(const table<T> &tab) const {
    size_t n1 = this->m_n_cols, n2 = tab.m_n_cols;
    size_t max_common_cols = std::min(n1, n2);
    vector<ptrdiff_t> concat_idx_to_var;
    concat_idx_to_var.reserve(n1 + n2);
    flat_hash_map<size_t, size_t> seen_vars;
    seen_vars.reserve(n1 + n2);
    vector<size_t> comm_idx1, comm_idx2;
    comm_idx1.reserve(max_common_cols);
    comm_idx2.reserve(max_common_cols);
    for (size_t i = 0; i < n1 + n2; ++i) {
      size_t var = (i < n1) ? this->idx_to_var[i] : tab.idx_to_var[i - n1];
      auto it = seen_vars.find(var);
      if (it == seen_vars.end()) {
        concat_idx_to_var.push_back(var);
        seen_vars[var] = i;
      } else {
        assert(i >= n1 && it->second < n1);
        comm_idx1.push_back(it->second);
        comm_idx2.push_back(i - n1);
        concat_idx_to_var.push_back(-1);
      }
    }

    flat_hash_map<vector<T>, vector<TblImplPtr>>
        hash_map;
    {
      size_t i = 0;
      for (const auto &row : tab.m_tab_impl) {
        vector<T> col_vals;
        col_vals.reserve(comm_idx2.size());
        for (const auto &idx : comm_idx2) {
          col_vals.push_back(row[idx]);
        }
        hash_map[col_vals].push_back(&row);
        ++i;
      }
    }
    table<T> new_tab;
    new_tab.m_n_cols = n1 + n2 - comm_idx1.size();
    new_tab.var_to_idx = this->var_to_idx;
    new_tab.idx_to_var = this->idx_to_var;
    new_tab.idx_to_var.reserve(n1 + n2);
    for (size_t i = 0, count = 0; i < n2; ++i) {
      if (concat_idx_to_var[n1 + i] != -1) {
        new_tab.idx_to_var.push_back(concat_idx_to_var[n1 + i]);
        new_tab.var_to_idx[concat_idx_to_var[n1 + i]] = n1 + i;
        count++;
      }
    }
    for (const auto &row : this->m_tab_impl) {
      vector<T> col_vals;
      col_vals.reserve(comm_idx1.size());
      for (const auto &idx : comm_idx1) {
        col_vals.push_back(row[idx]);
      }
      const auto it = hash_map.find(col_vals);
      if (it == hash_map.end())
        continue;
      for (const auto &row2_ptr : it->second) {
        vector<T> new_row = row;
        for (size_t i = 0; i < n2; ++i) {
          if (concat_idx_to_var[n1 + i] == -1)
            continue;
          else
            new_row.push_back((*row2_ptr)[i]);
        }
        new_tab.m_tab_impl.insert(std::move(new_row));
      }
    }
    return new_tab;
  }
  table<T> anti_join(const table<T> &tab);
  table<T> t_union(const table<T> &tab) const {
    vector<size_t> permutation;
    bool ret = this->find_permutation(tab, permutation);
    if (!ret)
      throw std::invalid_argument("table layouts are incompatible");
    table<T> new_tab(this->m_col_idxs);
    new_tab.m_tab_impl = this->m_tab_impl;
    for (const auto &row : tab.m_tab_impl) {
      vector<T> permuted_row = permute_row(permutation, row);
      new_tab.m_tab_impl.insert(permuted_row);
    }
    return new_tab;
  }

private:
  table() = default;
  /*
    Finds correct permutation of table indexes of the other table,
    returns false if the layouts of the tables are incompatible
  */
  bool find_permutation(const table<T> &other,
                        vector<size_t> &permutation) const {
    size_t n = this->m_n_cols;
    if (this->m_n_cols != other.m_n_cols)
      return false;
    permutation.reserve(n);
    for (size_t idx1 = 0; idx1 < n; ++idx1) {
      const auto it_idx2 = other.var_to_idx.find(this->idx_to_var[idx1]);
      if (it_idx2 == other.var_to_idx.end())
        return false;
      permutation.push_back(it_idx2->second);
    }
    return true;
  }
  static vector<T> permute_row(const vector<size_t> &permutation,
                               const vector<T> &row) {
    vector<T> permuted_row;
    permuted_row.reserve(row.size());
    for (const auto &idx : permutation) {
      permuted_row.push_back(row[idx]);
    }
    return permuted_row;
  }
  size_t m_n_cols;
  TblImplType m_tab_impl;
  vector<size_t> idx_to_var;
  Var2IdxMap var_to_idx;
};
} // namespace tbl_impl

template <typename T> struct fmt::formatter<table<T>> {
  constexpr auto parse(format_parse_context &ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    if (it != ctx.end() && *it != '}')
      throw format_error("invalid format - only empty format strings are "
                         "accepted for tables");
    return it;
  }
  template <typename FormatContext>
  auto format(const table<T> &tab, FormatContext &ctx) -> decltype(ctx.out()) {
    auto new_out =
        format_to(ctx.out(), "Table with {} columns\n", tab.m_n_cols);
    new_out = format_to(new_out, "{}\n", tab.idx_to_var);
    new_out = format_to(new_out, "Table Data:\n");
    for (const auto &row : tab.m_tab_impl) {
      new_out = format_to(new_out, "{}\n", row);
    }
    return new_out;
  }
};

#endif