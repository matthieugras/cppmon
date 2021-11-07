#ifndef CPPMON_SHM_EVENT_PRODUCER_H
#define CPPMON_SHM_EVENT_PRODUCER_H

#include <c_event_types.h>

#ifdef __cplusplus
namespace ipc {
class event_source;
}
using ev_src_ctxt = ipc::event_source;
#define EXPORT_C extern "C"
#include <cstddef>
#else
#define EXPORT_C
#include <stddef.h>
typedef struct ev_src_ctxt ev_src_ctxt;
#endif

// ================================= C API =================================  //

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int log_to_file : 1;
  int log_to_stdout : 1;
} ev_src_init_flags;

typedef struct {
  ev_src_init_flags flags;
  char *log_path;
  char *shm_id;
} ev_src_init_opts;

ev_src_ctxt *ev_src_init(const ev_src_init_opts *options);
void ev_src_free(ev_src_ctxt *ctx);
int ev_src_teardown(ev_src_ctxt *ctx);
int ev_src_add_db(ev_src_ctxt *ctx, size_t timestamp);
int ev_src_add_ev(ev_src_ctxt *ctx, char *ev_name, const c_ev_ty *ev_types,
                  const c_ev_data *data, size_t arity);
const char *ev_src_last_err(ev_src_ctxt *ctx);

#ifdef __cplusplus
}
#endif

// ================================ C++ API ================================  //

#ifdef __cplusplus
#include <boost/interprocess/managed_shared_memory.hpp>
#include <fmt/format.h>
#include <fmt/os.h>
#include <optional>
#include <shm_shared_state.h>
#include <span>
#include <string>
#include <string_view>

namespace ipc {
class event_source {
public:
  event_source(bool log_to_file, bool log_to_stdout, std::string shm_id,
               std::string log_path = "output.log");

  void set_error(std::string s);
  const char *get_error() const;
  void terminate();
  void add_database(size_t timestamp);
  void add_event(std::string_view name, std::span<const c_ev_ty> tys,
                 std::span<const c_ev_data> data);

private:
  void print_db();
  void print_event_data(c_ev_ty ty, const c_ev_data &data);
  void print_event(std::string_view name, std::span<const c_ev_ty> tys,
                   std::span<const c_ev_data> data);
  std::string last_error_;
  std::string curr_db_;
  size_t events_in_db_;
  std::optional<fmt::ostream> log_file_;
  boost_ipc::managed_shared_memory segment_;
  shared_state *state_;
  bool do_log_;
};
}// namespace ipc
#endif


#endif// CPPMON_SHM_EVENT_PRODUCER_H
