#include <database.h>

namespace monitor {
database monitor_db_from_parser_db(parse::database &&db) {
  database ret_db;
  ret_db.reserve(db.size());
  for (auto &entry : db) {
    if (entry.second.empty())
      continue;
    ret_db.emplace(entry.first, make_vector(std::move(entry.second)));
  }
  return ret_db;
}
}// namespace monitor
