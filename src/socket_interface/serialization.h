#ifndef CPPMON_SERIALIZATION_H
#define CPPMON_SERIALIZATION_H
#include <SPSCQueue.h>
#include <absl/time/clock.h>
#include <boost/container/devector.hpp>
#include <c_event_types.h>
#include <fmt/format.h>
#include <future>
#include <list>
#include <memory>
#include <mutex>
#include <optional>
#include <serialization_types.h>
#include <string_view>
#include <queue>
#include <thread>
#include <vector>
extern "C" {
#include <sys/eventfd.h>
#include <sys/uio.h>
}
#include <liburing.h>

namespace ipc::serialization {
using latency_cb_t = std::function<void(absl::Duration)>;

template<typename F, typename... Args>
int throw_on_err(F f, std::string_view err_str, Args &&...args) {
  int res = f(std::forward<Args>(args)...);
  if (res < 0) {
    int errno_save = errno;
    throw std::runtime_error(
      fmt::format("error: {} ({})", err_str, strerror(errno_save)));
  }
  return res;
}

/* F takes an int + pointer to user data and returns true iff it is not done
 */
template<typename F>
int for_each_ring_event(F f, io_uring *ring) {
  io_uring_cqe *cqe;
  while (true) {
    int ret = io_uring_wait_cqe(ring, &cqe);
    if (ret < 0) {
      errno = -ret;
      return -1;
    }
    if (cqe->res < 0) {
      errno = -cqe->res;
      return -1;
    }
    if (!f(static_cast<int>(cqe->res),
           reinterpret_cast<void *>(cqe->user_data))) {
      io_uring_cqe_seen(ring, cqe);
      break;
    }
    io_uring_cqe_seen(ring, cqe);
  }
  return 0;
}

template<typename... Args>
void log_to_stderr(fmt::format_string<Args...> fmt, Args &&...args) {
#ifndef NDEBUG
  auto t_now = absl::FormatTime(absl::Now(), absl::LocalTimeZone());
  auto msg = fmt::format(fmt, std::forward<Args>(args)...);
  fmt::print(stderr, "At time {}. {}\n", t_now, msg);
#endif
}

class serializer {
public:
  // Functions called from main thread
  serializer(const std::string &socket_path, size_t wbuf_size, latency_cb_t cb);
  ~serializer() noexcept;
  void chunk_latency_marker();
  void chunk_string(const char *str);
  void chunk_event_data(c_ev_ty ty, const c_ev_data &data);
  void chunk_event(const char *name, const c_ev_ty *tys, const c_ev_data *data,
                   size_t arity);
  void chunk_end_db();
  void chunk_begin_db(size_t ts);
  void chunk_terminate();

private:
  // Helper functions functions called from main thread
  void chunk_raw_data(const char *data, size_t len);
  void run_io_event_loop();
  void write_chunk_to_queue(bool is_last = false);
  template<typename T>
  void chunk_primitive(T val) {
    char val_as_bytes[sizeof(T)];
    std::memcpy(val_as_bytes, &val, sizeof(T));
    chunk_raw_data(val_as_bytes, sizeof(T));
  }

  // Helper functions called from IO thread
  enum submit_type {
    NEW_DATA_EVENT = 0x1,
    DATA_WRITE,
    DATA_READ,
    SHUTDOWN_WRITE
  };
  static void *st_to_ptr(submit_type ty);
  static submit_type st_from_ptr(void *ty);
  void read_chunks_from_queue(size_t nchunks);
  void submit_eventfd_read(eventfd_t *buf, int fd);
  void submit_cppmon_read(size_t nbytes, size_t offset = 0);
  void submit_shutdown_write_sock();
  bool maybe_read_more(size_t full_size);
  void maybe_submit_cppmon_write();
  void handle_read_done(size_t nbytes);
  void handle_write_done(size_t nbytes);
  void handle_new_data_eventfd();

  // Shared between threads
  struct q_elem {
    bool is_last;
    std::vector<char> data;
  };

  std::queue<q_elem> dat_q_;
  std::mutex mut_;
  int new_dat_fd_ = -1;

  // Owned by IO thread
  std::array<char, 50> rbuf_;
  size_t curr_read_bytes_ = 0;
  size_t new_event_sum_ = 0;
  std::list<std::vector<char>> wbufs_;
  boost::container::devector<iovec> wbufs_views_;
  size_t min_buf_size_;
  io_uring ring;
  bool waiting_for_lm_ = false;
  bool done_confirmed_ = false;
  bool write_pending_ = false;
  bool no_more_data_ = false;
  int sock_fd_ = -1;
  eventfd_t new_dat_;
  latency_cb_t cb_;

  // Owned by main thread
  std::optional<std::future<void>> receiver_;
  std::vector<char> chunk_buf_;
};
}// namespace ipc::serialization

#endif// CPPMON_SERIALIZATION_H
