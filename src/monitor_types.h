#ifndef CPPMON_MONITOR_TYPES_H
#define CPPMON_MONITOR_TYPES_H

#include <absl/container/flat_hash_map.h>
#include <buffers.h>
#include <event_data.h>
#include <limits>
#include <string>
#include <table.h>
#include <traceparser.h>
#include <utility>
#include <vector>
#include <optional>

namespace monitor {
using event_table = table<common::event_data>;
using opt_table = std::optional<event_table>;
using event_table_vec = std::vector<opt_table>;
// (ts, tp, data)
using satisfactions = std::vector<
  std::tuple<size_t, size_t, std::vector<std::vector<common::event_data>>>>;
using event = std::vector<common::event_data>;
using binary_buffer = common::binary_buffer<opt_table>;
using ts_list = std::vector<size_t>;
inline constexpr size_t MAXIMUM_TIMESTAMP = std::numeric_limits<size_t>::max();
}// namespace monitor

#endif
