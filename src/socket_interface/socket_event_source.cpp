#include <cstdio>
#include <cstring>
#include <exception>
#include <socket_event_source.h>
#include <stdexcept>
#include <stdlib.h>

EXPORT_C ev_src_ctxt *ev_src_init(const ev_src_init_opts *options) {
  try {
    bool log_to_file = options->flags.log_to_file,
         log_to_stdout = options->flags.log_to_stdout;
    std::string uds_sock_path = options->uds_sock_path
                                  ? std::string(options->uds_sock_path)
                                  : "cppmon_uds";
    if (options->log_path)
      return new ipc::event_source(
        log_to_file, log_to_stdout, uds_sock_path, options->wbuf_size,
        options->flags.unbounded_buf, std::string(options->log_path));
    else
      return new ipc::event_source(log_to_file, log_to_stdout, uds_sock_path,
                                   options->wbuf_size,
                                   options->flags.unbounded_buf);
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

EXPORT_C int ev_src_add_ev(ev_src_ctxt *ctx, char *ev_name,
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

EXPORT_C const char *ev_src_last_err(ev_src_ctxt *ctx) {
  try {
    return ctx->get_error();
  } catch (...) {
    fmt::print(stderr, "failed to get the error string\n");
    std::abort();
  }
};

namespace ipc {

void event_source::set_error(std::string s) { last_error_ = std::move(s); }

const char *event_source::get_error() const {
  if (last_error_ == "")
    return nullptr;
  else
    return last_error_.c_str();
}

event_source::event_source(bool log_to_file, bool log_to_stdout,
                           const std::string &socket_path, size_t wbuf_size,
                           bool unbounded_buffer, std::string log_path)
    : events_in_db_(0), do_log_(false), at_least_one_db_(false),
      serial_(socket_path, wbuf_size, unbounded_buffer) {
  if (log_to_file || log_to_stdout)
    do_log_ = true;
  if (log_to_file)
    log_file_.emplace(fmt::output_file(log_path));
}

void event_source::terminate() {
  if (at_least_one_db_)
    serial_.send_end_db();
  serial_.send_terminate();

  if (do_log_)
    print_db();
}

void event_source::add_database(size_t timestamp) {
  if (at_least_one_db_)
    serial_.send_end_db();
  serial_.send_begin_db(timestamp);
  at_least_one_db_ = true;

  if (do_log_) {
    print_db();
    curr_db_str_ += fmt::format("@{} ", timestamp);
  }
}

void event_source::add_event(char *name, const c_ev_ty *tys,
                             const c_ev_data *data, size_t arity) {
  serial_.send_event(name, tys, data, arity);
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

void event_source::print_event(char *name, const c_ev_ty *tys,
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