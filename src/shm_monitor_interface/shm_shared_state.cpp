#include <cstring>
#include <shm_shared_state.h>
#include <stdexcept>

namespace ipc {

shared_state::shared_state(const void_allocator &alloc)
    : alloc_(alloc), buf_(alloc), mutex_(), cond_(), data_avail_(false),
      done_(false) {}

void shared_state::add_event(string &&name, event_data_list &&data) {
  scoped_lock lk(mutex_);
  if (buf_.empty())
    throw std::runtime_error("no database to append the event to.");
  buf_.back().second.push_back(std::pair(std::move(name), std::move(data)));
}

void shared_state::add_c_event(std::string_view name,
                               std::span<const c_ev_ty> tys,
                               std::span<const c_ev_data> data) {
  size_t arity = tys.size();
  assert(arity == data.size());
  event_data_list new_data(alloc_);
  new_data.reserve(arity);
  for (size_t i = 0; i < arity; ++i) {
    if (tys[i] == TY_INT) {
      new_data.emplace_back(data[i].i);
    } else if (tys[i] == TY_FLOAT) {
      new_data.emplace_back(data[i].d);
    } else if (tys[i] == TY_STRING) {
      new_data.emplace_back(string(data[i].s, alloc_));
    } else {
      throw std::runtime_error("invalid event type");
    }
  }
  add_event(string(name, alloc_), std::move(new_data));
}

void shared_state::start_new_database(size_t ts) {
  scoped_lock lk(mutex_);
  bool was_empty = buf_.empty();
  buf_.emplace_back(ts, database(alloc_));
  if (!was_empty) {
    data_avail_ = true;
    cond_.notify_one();
  }
}

std::pair<database_buffer, bool> shared_state::get_events() {
  database_buffer result(alloc_);
  scoped_lock lk(mutex_);
  while (!done_ && !data_avail_)
    cond_.wait(lk);
  assert(lk.owns());
  if (done_) {
    result.splice(buf_.end(), buf_);
    return std::pair(std::move(result), true);
  } else {
    assert(buf_.size() > 1);
    auto it_before_end = buf_.end();
    it_before_end--;
    result.splice(it_before_end, buf_);
    assert(!buf_.empty());
    return std::pair(std::move(result), false);
  }
}

void shared_state::terminate() {
  scoped_lock lk(mutex_);
  done_ = true;
  if (!buf_.empty()) {
    data_avail_ = true;
  }
  cond_.notify_one();
}

}// namespace ipc