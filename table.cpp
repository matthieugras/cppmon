#include <table.h>

namespace detail {
table_layout get_join_layout(const table_layout &l1, const table_layout &l2) {
  size_t n1 = l1.size(), n2 = l2.size();
  table_layout res;
  res.reserve(n1 + n2);
  flat_hash_set<size_t> seen_vars;
  seen_vars.reserve(n1);
  for (auto var : l1) {
    res.push_back(var);
    seen_vars.insert(var);
  }
  std::copy_if(
    l2.cbegin(), l2.cend(), std::back_inserter(res),
    [&seen_vars](const auto var) { return !seen_vars.template contains(var); });
  return res;
}
}// namespace tbl_impl