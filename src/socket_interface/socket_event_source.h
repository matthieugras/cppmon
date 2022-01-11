#ifndef CPPMON_SOCKET_EVENT_SOURCE_H
#define CPPMON_SOCKET_EVENT_SOURCE_H

#include <c_event_types.h>
#include <cstddef>

namespace ipc {
class event_source;
}
using ev_src_ctxt = ipc::event_source;
#define LINK_API __attribute__((visibility("default")))
#define EXPORT_C extern "C"

// ================================= C API =================================  //

extern "C" {
typedef struct {
  int log_to_file : 1;
  int log_to_stdout : 1;
  int online_monitoring : 1;
} ev_src_init_flags;

typedef struct {
  ev_src_init_flags flags;
  size_t wbuf_size;
  char *log_path;
  char *uds_sock_path;
  char *latency_tracking_path;
} ev_src_init_opts;

LINK_API ev_src_ctxt *ev_src_init(const ev_src_init_opts *options);
LINK_API void ev_src_free(ev_src_ctxt *ctx);
LINK_API int ev_src_teardown(ev_src_ctxt *ctx);
LINK_API int ev_src_add_db(ev_src_ctxt *ctx, size_t timestamp);
LINK_API int ev_src_add_ev(ev_src_ctxt *ctx, const char *ev_name,
                           const c_ev_ty *ev_types, const c_ev_data *data,
                           size_t arity);
LINK_API int ev_src_add_singleton_db(ev_src_ctxt *ctx, size_t timestamp,
                                     const char *ev_name,
                                     const c_ev_ty *ev_types,
                                     const c_ev_data *data, size_t arity);
LINK_API const char *ev_src_last_err(ev_src_ctxt *ctx);
}

// ================================ C++ API ================================  //

#include <absl/time/clock.h>
#include <boost/system/error_code.hpp>
#include <fmt/format.h>
#include <fmt/os.h>
#include <optional>
#include <serialization.h>
#include <span>
#include <string>
#include <string_view>

namespace ipc {

class event_source {
public:
  event_source(bool log_to_file, bool log_to_stdout, bool online_monitoring,
               const std::string &socket_path, size_t wbuf_size,
               const std::string &log_path,
               std::optional<std::string> lat_track_path);

  void set_error(std::string s);
  const char *get_error() const;
  void terminate();
  void add_database(size_t timestamp);
  void add_event(const char *name, const c_ev_ty *tys, const c_ev_data *data,
                 size_t arity);

private:
  void handle_latency_marker(absl::Duration d);
  void print_db();
  void print_event_data(c_ev_ty ty, const c_ev_data &data);
  void print_event(const char *name, const c_ev_ty *tys, const c_ev_data *data,
                   size_t arity);

  std::string last_error_;
  std::string curr_db_str_;
  size_t events_in_db_;
  std::optional<fmt::ostream> log_file_;
  bool do_log_, at_least_one_db_;
  size_t last_marker_;
  std::optional<serialization::serializer> serial_;
  std::optional<fmt::ostream> lat_out_;
};
}// namespace ipc

#endif// CPPMON_SOCKET_EVENT_SOURCE_H
