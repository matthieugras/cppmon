#ifndef CPPMON_SHM_EVENT_SOURCE_H
#define CPPMON_SHM_EVENT_SOURCE_H

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
} ev_src_init_flags;

typedef struct {
  ev_src_init_flags flags;
  char *log_path;
  char *uds_sock_path;
} ev_src_init_opts;

LINK_API ev_src_ctxt *ev_src_init(const ev_src_init_opts *options);
LINK_API void ev_src_free(ev_src_ctxt *ctx);
LINK_API int ev_src_teardown(ev_src_ctxt *ctx);
LINK_API int ev_src_add_db(ev_src_ctxt *ctx, size_t timestamp);
LINK_API int ev_src_add_ev(ev_src_ctxt *ctx, char *ev_name,
                           const c_ev_ty *ev_types, const c_ev_data *data,
                           size_t arity);
LINK_API const char *ev_src_last_err(ev_src_ctxt *ctx);
}

// ================================ C++ API ================================  //

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
  event_source(bool log_to_file, bool log_to_stdout,
               const std::string &socket_path,
               std::string log_path = "output.log");

  void set_error(std::string s);
  const char *get_error() const;
  void terminate();
  void add_database(size_t timestamp);
  void add_event(char *name, const c_ev_ty *tys, const c_ev_data *data,
                 size_t arity);

private:
  void print_db();
  void print_event_data(c_ev_ty ty, const c_ev_data &data);
  void print_event(char *name, const c_ev_ty *tys, const c_ev_data *data,
                   size_t arity);

  std::string last_error_;
  std::string curr_db_str_;
  size_t events_in_db_;
  std::optional<fmt::ostream> log_file_;
  bool do_log_, at_least_one_db_;
  serialization::serializer serial_;
};
}// namespace ipc

#endif// CPPMON_SHM_EVENT_SOURCE_H
