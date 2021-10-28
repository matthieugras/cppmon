#include "since_impl.h"
#include <fmt/core.h>

namespace monitor::detail {
since_impl::since_impl(bool left_negated, size_t nfvs,
                       std::vector<size_t> comm_idx_r, fo::Interval inter)
    : since_base<shared_base<shared_no_agg<since_impl>>>(
        left_negated, std::move(comm_idx_r), nfvs, inter) {}

once_agg_impl::once_agg_impl(
  size_t nfvs, fo::Interval inter,
  agg_temporal::temporal_aggregation_impl temporal_agg)
    : once_base<shared_base<shared_agg_base<once_impl>>>(
        nfvs, inter, std::move(temporal_agg)){};

since_agg_impl::since_agg_impl(
  bool left_negated, size_t nfvs, std::vector<size_t> comm_idx_r,
  fo::Interval inter, agg_temporal::temporal_aggregation_impl temporal_agg)
    : since_base<shared_base<shared_agg_base<since_agg_impl>>>(
        left_negated, std::move(comm_idx_r), nfvs, inter,
        std::move(temporal_agg)) {}

once_impl::once_impl(size_t nfvs, fo::Interval inter)
    : once_base<shared_base<shared_no_agg<once_impl>>>(
        nfvs, inter) {}
}// namespace monitor::detail
