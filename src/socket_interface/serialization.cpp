#include <cstring>
#include <memory>
#include <serialization.h>
#include <stdexcept>
#include <utility>
extern "C" {
#include <sys/eventfd.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>
}

namespace ipc::serialization {

serializer::serializer(const std::string &socket_path, size_t wbuf_size,
                       latency_cb_t cb)
    : dat_q_(10000), min_buf_size_(std::min(1024UL, wbuf_size)),
      cb_(std::move(cb)) {
  ev_done_fd_ = throw_on_err(eventfd, "failed to create fd", 0u, 0);
  new_dat_fd_ = throw_on_err(eventfd, "failed to create fd", 0u, 0);
  sock_fd_ =
    throw_on_err(socket, "failed to create socket", AF_UNIX, SOCK_STREAM, 0);
  sockaddr_un addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.sun_family = AF_UNIX;
  std::strcpy(addr.sun_path, socket_path.c_str());
  throw_on_err(connect, "failed to connect to cppmon", sock_fd_,
               reinterpret_cast<sockaddr *>(&addr),
               static_cast<unsigned int>(sizeof(addr)));
  throw_on_err(io_uring_queue_init, "failed to init uring", 100u, &ring, 0u);
  receiver_.emplace(
    std::async(std::launch::async, &serializer::run_io_event_loop, this));
}

serializer::~serializer() noexcept {
  close(ev_done_fd_);
  close(new_dat_fd_);
  close(sock_fd_);
  io_uring_queue_exit(&ring);
}

void *serializer::st_to_ptr(submit_type ty) {
  return reinterpret_cast<void *>(ty);
}

serializer::submit_type serializer::st_from_ptr(void *ty) {
  return static_cast<submit_type>(reinterpret_cast<intptr_t>(ty));
}

void serializer::submit_shutdown_write_sock() {
  io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  if (!sqe)
    throw std::runtime_error("liburing submission queue is full!");
  io_uring_prep_shutdown(sqe, sock_fd_, SHUT_WR);
  io_uring_sqe_set_data(sqe, st_to_ptr(SHUTDOWN_WRITE));
  io_uring_submit(&ring);
}

void serializer::submit_eventfd_read(eventfd_t *buf, int fd) {
  io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  if (!sqe)
    throw std::runtime_error("liburing submission queue is full!");
  assert(ev_done_fd_ >= 0 && new_dat_fd_ >= 0);
  submit_type user_dat;
  if (fd == ev_done_fd_) {
    user_dat = FINISHED_EVENT;
  } else if (fd == new_dat_fd_) {
    user_dat = NEW_DATA_EVENT;
  } else {
    throw std::runtime_error("unknown fd");
  }
  io_uring_prep_read(sqe, fd, buf, sizeof(eventfd_t), 0);
  io_uring_sqe_set_data(sqe, st_to_ptr(user_dat));
  io_uring_submit(&ring);
}

void serializer::submit_cppmon_read(size_t nbytes, size_t offset) {
  assert(offset + nbytes < rbuf_.size());
  io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  if (!sqe)
    throw std::runtime_error("liburing submission queue is full");
  io_uring_prep_read(sqe, sock_fd_, rbuf_.data() + offset,
                     static_cast<unsigned int>(nbytes), 0);
  io_uring_sqe_set_data(sqe, st_to_ptr(DATA_READ));
  io_uring_submit(&ring);
}

void serializer::maybe_submit_cppmon_write() {
  assert(!write_pending_);
  if (!got_finished_event_ && wbufs_.size() < min_buf_size_) {
    submit_eventfd_read(&new_dat_, new_dat_fd_);
    return;
  }
  if (got_finished_event_ && wbufs_.empty()) {
    submit_shutdown_write_sock();
    return;
  }
  size_t num_vecs = std::min(wbufs_views_.size(), 1024UL);
  assert(!wbufs_.empty());
  assert(wbufs_.size() == wbufs_views_.size());
  write_pending_ = true;
  io_uring_sqe *sqe = io_uring_get_sqe(&ring);
  if (!sqe)
    throw std::runtime_error("liburing submission queue is full");
  io_uring_prep_writev(sqe, sock_fd_, wbufs_views_.data(),
                       static_cast<unsigned int>(num_vecs), 0);
  io_uring_sqe_set_data(sqe, st_to_ptr(DATA_WRITE));
  io_uring_submit(&ring);
}

void serializer::handle_write_done(size_t nbytes) {
  write_pending_ = false;
  size_t out_written = 0;
  for (size_t prefix_sum = 0; !wbufs_.empty();
       wbufs_.pop_front(), wbufs_views_.pop_front()) {
    assert(!wbufs_views_.empty());
    assert(prefix_sum <= nbytes);
    prefix_sum += wbufs_views_.front().iov_len;
    if (prefix_sum > nbytes) {
      auto &iov = wbufs_views_.front();
      size_t nb_missing = prefix_sum - nbytes;
      assert(nb_missing <= iov.iov_len);
      size_t nb_done = iov.iov_len - nb_missing;
      iov.iov_len -= nb_done;
      char *base_as_char = static_cast<char *>(iov.iov_base);
      base_as_char += nb_done;
      iov.iov_base = base_as_char;
      break;
    } else {
      out_written++;
    }
  }
  maybe_submit_cppmon_write();
}

void serializer::handle_new_data_eventfd() {
  if (got_finished_event_)
    return;
  read_chunks_from_queue(new_dat_);
  maybe_submit_cppmon_write();
}

bool serializer::maybe_read_more(size_t full_size) {
  if (curr_read_bytes_ < full_size) {
    submit_cppmon_read(full_size - curr_read_bytes_, curr_read_bytes_);
    return true;
  }
  curr_read_bytes_ = 0;
  return false;
}

void serializer::handle_read_done(size_t nbytes) {
  if (!nbytes)
    throw std::runtime_error("unexpected eof");
  curr_read_bytes_ += nbytes;
  if (waiting_for_lm_) {
    assert(curr_read_bytes_ <= sizeof(int64_t));
    if (maybe_read_more(sizeof(int64_t)))
      return;
    int64_t latency_marker;
    std::memcpy(&latency_marker, rbuf_.data(), sizeof(int64_t));
    auto latency = absl::Now() - absl::FromUnixNanos(latency_marker);
    if (cb_)
      cb_(latency);
    submit_cppmon_read(sizeof(control_bits));
    waiting_for_lm_ = false;
  } else {
    assert(curr_read_bytes_ <= sizeof(control_bits));
    if (maybe_read_more(sizeof(control_bits)))
      return;
    control_bits ctrl;
    std::memcpy(&ctrl, rbuf_.data(), sizeof(control_bits));
    switch (ctrl) {
      case CTRL_LATENCY_MARKER:
        waiting_for_lm_ = true;
        submit_cppmon_read(sizeof(int64_t));
        break;
      case CTRL_EOF:
        done_confirmed_ = true;
        break;
      default:
        throw std::runtime_error("got unexpected command from cppmon");
    }
  }
}

void serializer::read_chunks_from_queue(size_t nchunks) {
  for (size_t left = nchunks; left > 0; --left, dat_q_.pop()) {
    assert(!dat_q_.empty());
    auto &placed_elem = wbufs_.emplace_back(std::move(*dat_q_.front()));
    wbufs_views_.push_back(
      iovec{.iov_base = placed_elem.data(), .iov_len = placed_elem.size()});
  }
}

void serializer::handle_finished_eventfd() {
  if (got_finished_event_)
    return;
  got_finished_event_ = true;
  if (!write_pending_) {
    read_chunks_from_queue(dat_q_.size());
    maybe_submit_cppmon_write();
  }
}

void serializer::run_io_event_loop() {
  // try {
  submit_eventfd_read(&ev_done_, ev_done_fd_);
  submit_eventfd_read(&new_dat_, new_dat_fd_);
  submit_cppmon_read(sizeof(control_bits));
  size_t it_num = 0;
  auto fn1 = [this, &it_num](int res, void *data) {
    submit_type user_dat = st_from_ptr(data);
    it_num++;
    switch (user_dat) {
      case FINISHED_EVENT:
        handle_finished_eventfd();
        return true;
      case NEW_DATA_EVENT:
        handle_new_data_eventfd();
        return true;
      case DATA_WRITE:
        assert(res > 0);
        handle_write_done(static_cast<size_t>(res));
        return true;
      case DATA_READ:
        handle_read_done(static_cast<size_t>(res));
        if (got_finished_event_ && done_confirmed_)
          return false;
        else
          return true;
      case SHUTDOWN_WRITE:
        return true;
    }
  };
  throw_on_err(for_each_ring_event<decltype(fn1)>,
               "error processing uring events", fn1, &ring);
  /* } catch (const std::exception &e) {
    fmt::print(stderr, "exception occured: {}\n", e.what());
    throw;
  } */
}

void serializer::chunk_raw_data(const char *data, size_t len) {
  chunk_buf_.insert(chunk_buf_.end(), data, data + len);
}

void serializer::write_chunk_to_queue() {
  std::vector<char> swap_dat;
  swap_dat.reserve(chunk_buf_.size() * 2);
  swap_dat.swap(chunk_buf_);
  dat_q_.emplace(std::move(swap_dat));
  throw_on_err(eventfd_write, "failed to flush buffer", new_dat_fd_, 1u);
}

void serializer::shutdown_reader() {
  throw_on_err(eventfd_write, "failed to send shutdown to IO thread",
               ev_done_fd_, 1u);
}

void serializer::chunk_latency_marker() {
  int64_t now_as_unix = absl::ToUnixNanos(absl::Now());
  chunk_primitive(CTRL_LATENCY_MARKER);
  chunk_primitive(now_as_unix);
  write_chunk_to_queue();
}

void serializer::chunk_string(const char *str) {
  size_t len = strlen(str);
  chunk_primitive(len);
  chunk_raw_data(str, len);
}

void serializer::chunk_event_data(c_ev_ty ty, const c_ev_data &data) {
  chunk_primitive(ty);
  if (ty == TY_INT)
    chunk_primitive(data.i);
  else if (ty == TY_FLOAT)
    chunk_primitive(data.d);
  else if (ty == TY_STRING)
    chunk_string(data.s);
  else
    throw std::runtime_error("invalid type, expected int|float|string");
}

void serializer::chunk_event(const char *name, const c_ev_ty *tys,
                             const c_ev_data *data, size_t arity) {
  chunk_primitive(CTRL_NEW_EVENT);
  chunk_string(name);
  chunk_primitive(arity);
  for (size_t i = 0; i < arity; ++i)
    chunk_event_data(tys[i], data[i]);
}

void serializer::chunk_end_db() {
  chunk_primitive(CTRL_END_DATABASE);
  write_chunk_to_queue();
}

void serializer::chunk_begin_db(size_t ts) {
  chunk_primitive(CTRL_NEW_DATABASE);
  chunk_primitive(ts);
}

void serializer::chunk_terminate() {
  chunk_primitive(CTRL_EOF);
  write_chunk_to_queue();
  shutdown_reader();
  receiver_->get();
}
}// namespace ipc::serialization
