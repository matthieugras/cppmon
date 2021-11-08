#include <cppmon_event_source.h>
#include <cstdio>
#include <exception>
#include <stdexcept>
#include <stdlib.h>

EXPORT_C ev_src_ctxt *ev_src_init(const ev_src_init_opts *options) {
  try {
    bool log_to_file = options->flags.log_to_file,
         log_to_stdout = options->flags.log_to_stdout;
    std::string shm_id =
      options->shm_id ? std::string(options->shm_id) : "cppmon_shm";
    if (options->log_path)
      return new ipc::event_source(log_to_file, log_to_stdout, shm_id,
                                   std::string(options->log_path));
    else
      return new ipc::event_source(log_to_file, log_to_stdout, shm_id);
  } catch (const std::exception &e) {
    fmt::print(stderr,
               "failed to initialize context because of exception: {}\n",
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
    ctx->add_event(std::string_view(ev_name), std::span(ev_types, arity),
                   std::span(data, arity));
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
                           std::string shm_id, std::string log_path)
    : events_in_db_(0), segment_(boost_ipc::open_only, shm_id.data()),
      state_(nullptr), do_log_(false) {
  if (log_to_file || log_to_stdout)
    do_log_ = true;
  if (log_to_file)
    log_file_.emplace(fmt::output_file(log_path));
  auto [ptr, count] = segment_.find<shared_state>(boost_ipc::unique_instance);
  if (count != 1)
    throw std::runtime_error("shared memory segment not found ... cppmon is "
                             "started and uses same shm_id?");
  state_ = ptr;
}

void event_source::terminate() {
  if (do_log_)
    print_db();
  state_->terminate();
}

void event_source::print_db() {
  if (!curr_db_.empty()) {
    if (events_in_db_ != 0)
      curr_db_.pop_back();
    if (log_file_)
      log_file_->print("{};\n", curr_db_);
    else
      fmt::print("{};\n", curr_db_);
  }
  curr_db_.clear();
  events_in_db_ = 0;
}

void event_source::add_database(size_t timestamp) {
  if (do_log_) {
    print_db();
    curr_db_ += fmt::format("@{} ", timestamp);
  }
  state_->start_new_database(timestamp);
}

void event_source::print_event_data(c_ev_ty ty, const c_ev_data &data) {
  if (ty == TY_INT)
    curr_db_ += fmt::format("{},", data.i);
  else if (ty == TY_FLOAT)
    curr_db_ += fmt::format("{},", data.d);
  else
    curr_db_ += fmt::format("{},", data.s);
}

void event_source::print_event(std::string_view name,
                               std::span<const c_ev_ty> tys,
                               std::span<const c_ev_data> data) {
  assert(tys.size() == data.size());
  size_t arity = tys.size();
  curr_db_ += fmt::format("{}(", name);
  for (size_t i = 0; i < arity; ++i)
    print_event_data(tys[i], data[i]);
  if (arity != 0)
    curr_db_.pop_back();
  curr_db_ += ") ";
  events_in_db_++;
}

void event_source::add_event(std::string_view name,
                             std::span<const c_ev_ty> tys,
                             std::span<const c_ev_data> data) {
  if (do_log_)
    print_event(name, tys, data);
  state_->add_c_event(name, tys, data);
}
}// namespace ipc