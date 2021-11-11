#ifndef CPPMON_SERIALIZATION_H
#define CPPMON_SERIALIZATION_H

#include <boost/asio/buffered_write_stream.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/streambuf.hpp>
#include <c_event_types.h>
#include <condition_variable>
#include <fmt/format.h>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <serialization_types.h>
#include <thread>
#include <vector>

namespace ipc::serialization {

class serializer {
public:
  serializer(const std::string &socket_path, size_t wbuf_size,
             bool unbounded_buffering = false);
  void send_string(const char *str);
  void send_event_data(c_ev_ty ty, const c_ev_data &data);
  void send_event(const char *name, const c_ev_ty *tys, const c_ev_data *data,
                  size_t arity);
  void send_end_db();
  void send_begin_db(size_t ts);
  void send_terminate();

private:
  void worker_fun();
  void send_to_buffer(const char *data, size_t len);
  template<typename T>
  void send_primitive(T val) {
    char val_as_bytes[sizeof(T)];
    std::memcpy(val_as_bytes, &val, sizeof(T));
    if (unbounded_buf_)
      send_to_buffer(val_as_bytes, sizeof(T));
    else
      boost::asio::write(wstream_,
                         boost::asio::const_buffer(val_as_bytes, sizeof(T)));
  }
  size_t wbuf_size_;
  boost::asio::io_context ctx_;
  boost::asio::buffered_write_stream<
    boost::asio::local::stream_protocol::socket>
    wstream_;
  boost::asio::local::stream_protocol::socket sock_;
  boost::asio::streambuf buf_;
  std::mutex buf_mut_;
  std::condition_variable cond_;
  bool unbounded_buf_, has_data_, is_done_, should_read_;
  std::optional<std::future<void>> worker_;
};
}// namespace ipc::serialization

#endif// CPPMON_SERIALIZATION_H
