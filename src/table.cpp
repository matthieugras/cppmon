#include <absl/container/flat_hash_map.h>
#include <table.h>
#include <tuple>


namespace detail {

static absl::flat_hash_map<size_t, size_t>
get_var_2_idx(const table_layout &l) {
  size_t n = l.size();
  absl::flat_hash_map<size_t, size_t> res;
  res.reserve(n);
  for (size_t i = 0; i < n; ++i) {
    res[l[i]] = i;
  }
  assert(res.size() == n);
  return res;
}

join_info get_join_info(const table_layout &l1, const table_layout &l2) {
  size_t n1 = l1.size(), n2 = l2.size();
  vector<size_t> comm_idx1, comm_idx2;
  vector<size_t> keep_idx2;
  table_layout layout;
  auto var2idx1 = get_var_2_idx(l1), var2idx2 = get_var_2_idx(l2);
  for (auto var1 : l1) {
    layout.push_back(var1);
    if (var2idx2.contains(var1)) {
      comm_idx1.push_back(var2idx1[var1]);
      comm_idx2.push_back(var2idx2[var1]);
    }
  }
  for (auto var2 : l2) {
    if (!var2idx1.contains(var2)) {
      layout.push_back(var2);
      keep_idx2.push_back(var2idx2[var2]);
    }
  }
  assert(comm_idx1.size() == comm_idx2.size());
  assert(layout.size() == (n1 + n2 - comm_idx1.size()));
  assert(layout.size() == (n1 + n2 - comm_idx2.size()));
  return join_info{std::move(comm_idx1), std::move(comm_idx2),
                   std::move(keep_idx2), std::move(layout)};
}

anti_join_info get_anti_join_info(const table_layout &l1,
                                  const table_layout &l2) {
  vector<size_t> comm_idx1, comm_idx2;
  auto var2idx1 = get_var_2_idx(l1), var2idx2 = get_var_2_idx(l2);
  for (auto var1 : l1) {
    if (var2idx2.contains(var1)) {
      comm_idx1.push_back(var2idx1[var1]);
      comm_idx2.push_back(var2idx2[var1]);
    }
  }
  assert(comm_idx1.size() == comm_idx2.size());
  return anti_join_info{std::move(comm_idx1), std::move(comm_idx2), l1};
}

vector<size_t> id_permutation(size_t n) {
  vector<size_t> res(n);
  std::iota(res.begin(), res.end(), 0UL);
  return res;
}

// Find permutation of layout l2, so that is matches layout 1
vector<size_t> find_permutation(const table_layout &l1,
                                const table_layout &l2) {

  assert(l1.size() == l2.size());
  size_t n = l1.size();
  auto var_2_idx2 = get_var_2_idx(l2);
  vector<size_t> permutation;
  permutation.reserve(n);
  for (const auto &var : l1) {
    assert(var_2_idx2.contains(var));
    permutation.push_back(var_2_idx2.find(var)->second);
  }
  assert(permutation.size() == n);
  return permutation;
}
}// namespace detail