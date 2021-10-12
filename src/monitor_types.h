#ifndef CPPMON_MONITOR_TYPES_H
#define CPPMON_MONITOR_TYPES_H

#include <buffers.h>
#include <event_data.h>
#include <table.h>
#include <traceparser.h>
#include <vector>

namespace monitor {
using event_table = table<common::event_data>;
using event_table_vec = std::vector<event_table>;
// (ts, tp, data)
using satisfactions = std::vector<
  std::tuple<size_t, size_t, std::vector<std::vector<common::event_data>>>>;
using database = parse::database;
using binary_buffer = common::binary_buffer<event_table>;
}// namespace monitor

#endif
