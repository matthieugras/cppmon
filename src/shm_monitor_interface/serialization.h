#ifndef CPPMON_SERIALIZATION_H
#define CPPMON_SERIALIZATION_H

#include <boost/asio/buffered_write_stream.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <c_event_types.h>
#include <serialization_types.h>

namespace ipc::serialization {

class serializer {
public:
  serializer(const std::string &socket_path);
  void send_string(const char *str);
  void send_event_data(c_ev_ty ty, const c_ev_data &data);
  void send_event(const char *name, const c_ev_ty *tys, const c_ev_data *data,
                  size_t arity);
  void send_end_db();
  void send_begin_db(size_t ts);
  void send_terminate();

private:
  template<typename T>
  void send_primitive(T val) {
    char val_as_bytes[sizeof(T)];
    std::memcpy(val_as_bytes, &val, sizeof(T));
    boost::asio::write(wstream_,
                       boost::asio::const_buffer(val_as_bytes, sizeof(T)));
  }
  boost::asio::io_context ctx_;
  boost::asio::buffered_write_stream<
    boost::asio::local::stream_protocol::socket>
    wstream_;
};
}// namespace ipc::serialization

#endif// CPPMON_SERIALIZATION_H
