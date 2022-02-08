#ifndef CPPMON_DESERIALIZATION_H
#define CPPMON_DESERIALIZATION_H

#include <absl/container/flat_hash_map.h>
#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffered_read_stream.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/error_code.hpp>
#include <c_event_types.h>
#include <cstring>
#include <event_data.h>
#include <optional>
#include <serialization_types.h>
#include <string>
#include <traceparser.h>
#include <util.h>
#include <utility>

namespace ipc::serialization {
using database =
  absl::flat_hash_map<pred_id_t, std::vector<parse::database_elem>>;
using ts_database = std::pair<database, std::vector<size_t>>;

class deserializer {
public:
  deserializer(std::string socket_path, pred_map_t pred_map);
  std::optional<ts_database> read_database();
  void send_eof();
  ~deserializer();

private:
  void send_latency_marker(int64_t lm);
  template<typename T>
  T read_primitive() {
    T t{};
    std::array<char, sizeof(T)> buf;
    boost::asio::read(sock_, boost::asio::buffer(buf));
    std::memcpy(&t, buf.data(), sizeof(T));
    return t;
  }

  template<typename T>
  void send_primitive(T t) {
    char snd_buf[sizeof(T)];
    std::memcpy(snd_buf, &t, sizeof(T));
    boost::asio::write(sock_, boost::asio::const_buffer(snd_buf, sizeof(T)));
  }

  std::string read_string();
  common::event_data read_event_data();
  void read_tuple(database &db);
  void read_tuple_list(database &db);

  std::string path_;
  boost::asio::io_context ctx_;
  boost::asio::local::stream_protocol::acceptor acceptor_;
  boost::asio::buffered_read_stream<boost::asio::local::stream_protocol::socket>
    sock_;
  pred_map_t pred_map_;
};
}// namespace ipc::serialization

#endif// CPPMON_DESERIALIZATION_H
