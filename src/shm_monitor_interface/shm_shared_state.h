#ifndef CPPMON_SHM_SHARED_STATE_H
#define CPPMON_SHM_SHARED_STATE_H
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/containers/list.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/interprocess_condition.hpp>
#include <boost/interprocess/sync/interprocess_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/variant2/variant.hpp>
#include <cppmon_event_types_export.h>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace ipc {
namespace boost_ipc = boost::interprocess;
using segment_manager_t = boost_ipc::managed_shared_memory::segment_manager;
using void_allocator = boost_ipc::allocator<void, segment_manager_t>;
using char_allocator = boost_ipc::allocator<char, segment_manager_t>;
using string =
  boost_ipc::basic_string<char, std::char_traits<char>, char_allocator>;
using event_data = boost::variant2::variant<int, double, string>;
using event_data_allocator =
  boost_ipc::allocator<event_data, segment_manager_t>;
using event_data_list = boost_ipc::vector<event_data, event_data_allocator>;
// predicate_name x args
using event = std::pair<string, event_data_list>;
using event_allocator = boost_ipc::allocator<event, segment_manager_t>;
// event list
using database = boost_ipc::vector<event, event_allocator>;
using database_allocator = boost_ipc::allocator<database, segment_manager_t>;
using timestamped_database = std::pair<size_t, database>;
using timestamped_database_allocator =
  boost_ipc::allocator<timestamped_database, segment_manager_t>;
using database_buffer =
  boost_ipc::list<timestamped_database, timestamped_database_allocator>;
using mutex_t = boost_ipc::interprocess_mutex;
using condition_t = boost_ipc::interprocess_condition;
using scoped_lock = boost_ipc::scoped_lock<mutex_t>;

class shared_state {
public:
  shared_state(const void_allocator &alloc);
  void add_event(string &&name, event_data_list &&data);
  void add_c_event(std::string_view name, std::span<const c_ev_ty> tys,
                   std::span<const c_ev_data> data);
  std::pair<database_buffer, bool> get_events();
  void start_new_database(size_t ts);
  void terminate();

private:
  void_allocator alloc_;
  database_buffer buf_;
  mutex_t mutex_;
  condition_t cond_;
  bool data_avail_, done_;
};

}// namespace ipc

#endif// CPPMON_SHM_SHARED_STATE_H
