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

tuple<table_layout, vector<size_t>, vector<size_t>>
get_join_layout(const table_layout &l1, const table_layout &l2) {
  //TODO: fix this so that is supports antijoins
  size_t n1 = l1.size(), n2 = l2.size();
  vector<size_t> comm_idx1, comm_idx2;
  table_layout layout;
  comm_idx1.reserve(std::min(n1, n2));
  comm_idx2.reserve(std::min(n1, n2));
  layout.reserve(n1 + n2);
  auto var2idx1 = get_var_2_idx(l1), var2idx2 = get_var_2_idx(l2);
  for (auto var1 : l1) {
    layout.push_back(var1);
    if (var2idx2.contains(var1)) {
      comm_idx1.push_back(var2idx1[var1]);
    }
  }
  for (auto var2 : l2) {
    if (var2idx1.contains(var2)) {
      comm_idx2.push_back(var2idx2[var2]);
    } else {
      layout.push_back(var2);
    }
  }
  assert(comm_idx1.size() == comm_idx2.size());
  assert(layout.size() == (n1 + n2 - comm_idx1.size()));
  return {std::move(layout), std::move(comm_idx1), std::move(comm_idx2)};
}

vector<size_t> id_permutation(size_t n) {
  vector<size_t> res(n);
  std::iota(res.begin(), res.end(), 0UL);
  return res;
}

vector<size_t> find_permutation(const table_layout &l1,
                                const table_layout &l2) {

  assert(l1.size() == l2.size());
  size_t n = l1.size();
  absl::flat_hash_map<size_t, size_t> var_2_idx2;
  var_2_idx2.reserve(l2.size());
  for (size_t i = 0; i < n; ++i) {
    var_2_idx2[l2[i]] = i;
  }
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