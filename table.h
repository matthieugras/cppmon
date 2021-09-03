#ifndef TABLE_H
#define TABLE_H

#include <algorithm>
#include <boost/container_hash/extensions.hpp>
#include <boost/container_hash/hash.hpp>
#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

using std::pair;
using std::size_t;
using std::unordered_map;
using std::unordered_set;
using std::vector;

template <typename T> class table {
  using TblImplType = unordered_set<vector<T>, boost::hash<vector<T>>>;
  using TblImplPtr = typename TblImplType::const_pointer;

public:
  explicit table(const vector<size_t> &idx_to_var) {
    size_t n = idx_to_var.size();
    this->idx_to_var = idx_to_var;
    this->m_n_cols = n;
  }
  size_t tab_size() { return this->m_tab_impl.size(); }
  void add_row(const vector<T> &row) {
    assert(row.size() == this->m_n_cols);
    this->m_tab_impl.insert(row);
  }
  table<T> natural_join(const table<T> &tab) {
    size_t n1 = this->m_n_cols, n2 = tab.m_n_cols;
    size_t max_common_cols = std::min(n1, n2);
    vector<ptrdiff_t> concat_idx_to_var;
    concat_idx_to_var.reserve(n1 + n2);
    unordered_map<size_t, size_t> seen_vars;
    seen_vars.reserve(n1+n2);
    vector<size_t> comm_idx1, comm_idx2;
    comm_idx1.reserve(max_common_cols);
    comm_idx2.reserve(max_common_cols);
    for (size_t i = 0; i < n1+n2; ++i) {
      size_t var = (i < n1) ? this->idx_to_var[i] : tab.idx_to_var[i-n1];
      auto it = seen_vars.find(var);
      if (it == seen_vars.end()) {
        concat_idx_to_var.push_back(var);
        seen_vars[var] = i;
      } else {
        assert(i >= n1 && it->second < n1);
        comm_idx1.push_back(it->second);
        comm_idx2.push_back(i);
        concat_idx_to_var.push_back(-1);
      }
    }
    unordered_map<vector<T>, vector<TblImplPtr>, boost::hash<vector<T>>> hash_map;
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
    for (const auto &row : this->m_tab_impl) {
      vector<T> col_vals;
      col_vals.reserve(comm_idx1.size());
      for (const auto &idx : comm_idx1) {
        col_vals.push_back(row[idx]);
      }
      const auto it = hash_map.find(col_vals);
      if (it == hash_map.end())
        continue;
      for (const auto &row2_ptr: it->second) {
        vector<T> new_row = row;
        for (size_t i = 0; i < n2; ++i) {
          if (concat_idx_to_var[n1+i] == -1) continue;
          else new_row.push_back((*row2_ptr)[i]);
        }
      }
    }
  }
  table<T> anti_join(const table<T> &tab);
  table<T> t_union(const table<T> &tab) {
    /*assert(this->m_col_idxs == tab.m_col_idxs);
    table<T> tmp(this->m_col_idxs);
    tmp.m_tab_impl = this->m_tab_impl;
    // TODO: finish this*/
    return nullptr;
  }

private:
  table() = default;
  size_t m_n_cols;
  TblImplType m_tab_impl;
  vector<size_t> idx_to_var;
};

#endif