#ifndef CPPMON_DATABASE_H
#define CPPMON_DATABASE_H

#include <absl/container/flat_hash_map.h>
#include <string>
#include <traceparser.h>
#include <util.h>
#include <utility>

namespace monitor {
// map from (pred_name, arity) to lists of lists of events
using database = absl::flat_hash_map<std::pair<std::string, size_t>,
                                       std::vector<parse::database_elem>>;

database monitor_db_from_parser_db(parse::database &&db);

}// namespace monitor

#endif// CPPMON_DATABASE_H
