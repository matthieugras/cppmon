#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <socket_event_source.h>
#include <stdexcept>

EXPORT_C ev_src_ctxt *ev_src_init(const ev_src_init_opts *options) {
  try {
    bool log_to_file = options->flags.log_to_file,
         log_to_stdout = options->flags.log_to_stdout,
         online_monitoring = options->flags.online_monitoring;
    std::optional<std::string> lat_track_path =
      options->latency_tracking_path
        ? std::optional(options->latency_tracking_path)
        : std::nullopt;
    std::string uds_sock_path =
      options->uds_sock_path ? std::string(options->uds_sock_path) : "";
    std::string log_path =
      options->log_path ? std::string(options->log_path) : "";
    return new ipc::event_source(log_to_file, log_to_stdout, online_monitoring,
                                 uds_sock_path, options->wbuf_size, log_path,
                                 std::move(lat_track_path));
  } catch (const std::exception &e) {
    fmt::print(stderr,
               "failed to initialize context because of exception: {}\n"
               "Verify that cppmon is started...\n",
               e.what());
    std::abort();
  } catch (...) {
    fmt::print(stderr, "unkown error in intialization of context\n");
    std::abort();
  }
}

EXPORT_C void ev_src_free(ev_src_ctxt *ctx) { delete ctx; }

EXPORT_C int ev_src_teardown(ev_src_ctxt *ctx) {
  try {
    ctx->terminate();
    return 0;
  } catch (const std::exception &e) {
    ctx->set_error(
      fmt::format("failed to teardown because of exception: {}", e.what()));
  } catch (...) { ctx->set_error("unkown error in teardown of context"); }
  return -1;
}


EXPORT_C int ev_src_add_db(ev_src_ctxt *ctx, size_t timestamp) {
  try {
    ctx->add_database(timestamp);
    return 0;
  } catch (const std::exception &e) {
    ctx->set_error(
      fmt::format("failed to add database because of exception: {}", e.what()));
  } catch (...) { ctx->set_error("unkown error while adding database"); }
  return -1;
}

EXPORT_C int ev_src_add_ev(ev_src_ctxt *ctx, const char *ev_name,
                           const c_ev_ty *ev_types, const c_ev_data *data,
                           size_t arity) {
  try {
    assert(ev_types != nullptr && data != nullptr);
    ctx->add_event(ev_name, ev_types, data, arity);
    return 0;
  } catch (const std::exception &e) {
    ctx->set_error(
      fmt::format("failed to add event because of exception: {}", e.what()));
  } catch (...) { ctx->set_error("unknown error while adding event"); }

  return -1;
}

EXPORT_C int ev_src_add_singleton_db(ev_src_ctxt *ctx, size_t timestamp,
                                     const char *ev_name,
                                     const c_ev_ty *ev_types,
                                     const c_ev_data *data, size_t arity) {
  int ret;
  if ((ret = ev_src_add_db(ctx, timestamp)) < 0)
    return ret;
  if ((ret = ev_src_add_ev(ctx, ev_name, ev_types, data, arity)) < 0)
    return ret;
  return 0;
}

EXPORT_C const char *ev_src_last_err(ev_src_ctxt *ctx) {
  try {
    return ctx->get_error();
  } catch (...) {
    fmt::print(stderr, "failed to get the error string\n");
    std::abort();
  }
}

namespace ipc {

void event_source::set_error(std::string s) { last_error_ = std::move(s); }

const char *event_source::get_error() const {
  if (last_error_.empty())
    return nullptr;
  else
    return last_error_.c_str();
}

void event_source::handle_latency_marker(absl::Duration d) {
  auto now = absl::ToUnixMicros(absl::Now());
  lat_out_->print("{},{}\n", now, absl::ToInt64Microseconds(d));
  lat_out_->flush();
}

event_source::event_source(bool log_to_file, bool log_to_stdout,
                           bool online_monitoring,
                           const std::string &socket_path, size_t wbuf_size,
                           const std::string &log_path,
                           std::optional<std::string> lat_track_path)
    : events_in_db_(0), do_log_(false), at_least_one_db_(false),
      last_marker_(0) {
  if (log_to_file && log_to_stdout)
    throw std::runtime_error("logging to file + stdout is not supported");
  if (!log_to_file && !log_to_stdout && !online_monitoring)
    throw std::runtime_error(
      "offline monitoring without event logging does not make sense");
  if (log_to_file || log_to_stdout)
    do_log_ = true;
  if (log_to_file)
    log_file_.emplace(fmt::output_file(log_path));
  if (online_monitoring) {
    if (lat_track_path) {
      lat_out_.emplace(fmt::output_file(*lat_track_path));
      lat_out_->print("timestamp,latency\n");
      serial_.emplace(socket_path, wbuf_size,
                      std::bind(&event_source::handle_latency_marker, this,
                                std::placeholders::_1));
    } else {
      serial_.emplace(socket_path, wbuf_size, nullptr);
    }
  }
}

void event_source::terminate() {
  if (serial_) {
    if (at_least_one_db_)
      serial_->chunk_end_db();
    serial_->chunk_terminate();
  }

  if (do_log_)
    print_db();
}

void event_source::add_database(size_t timestamp) {
  if (serial_) {
    if (at_least_one_db_) {
      serial_->chunk_end_db();
      last_marker_++;
    }
    if (lat_out_ && last_marker_ >= 100) {
      last_marker_ = 0;
      serial_->chunk_latency_marker();
    }
    serial_->chunk_begin_db(timestamp);
    at_least_one_db_ = true;
  }

  if (do_log_) {
    print_db();
    curr_db_str_ += fmt::format("@{} ", timestamp);
  }
}

void event_source::add_event(const char *name, const c_ev_ty *tys,
                             const c_ev_data *data, size_t arity) {
  if (serial_)
    serial_->chunk_event(name, tys, data, arity);
  if (do_log_)
    print_event(name, tys, data, arity);
}

void event_source::print_db() {
  if (!curr_db_str_.empty()) {
    if (events_in_db_ != 0)
      curr_db_str_.pop_back();
    if (log_file_)
      log_file_->print("{};\n", curr_db_str_);
    else
      fmt::print("{};\n", curr_db_str_);
  }
  curr_db_str_.clear();
  events_in_db_ = 0;
}

void event_source::print_event_data(c_ev_ty ty, const c_ev_data &data) {
  if (ty == TY_INT)
    curr_db_str_ += fmt::format("{},", data.i);
  else if (ty == TY_FLOAT)
    curr_db_str_ += fmt::format("{},", data.d);
  else
    curr_db_str_ += fmt::format("\"{}\",", data.s);
}

void event_source::print_event(const char *name, const c_ev_ty *tys,
                               const c_ev_data *data, size_t arity) {
  curr_db_str_ += fmt::format("{}(", name);
  for (size_t i = 0; i < arity; ++i)
    print_event_data(tys[i], data[i]);
  if (arity != 0)
    curr_db_str_.pop_back();
  curr_db_str_ += ") ";
  events_in_db_++;
}
}// namespace ipc
