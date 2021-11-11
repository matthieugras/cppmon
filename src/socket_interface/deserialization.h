#ifndef CPPMON_DESERIALIZATION_H
#define CPPMON_DESERIALIZATION_H

#include <absl/container/flat_hash_map.h>
#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffered_read_stream.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/read.hpp>
#include <boost/system/error_code.hpp>
#include <c_event_types.h>
#include <cstring>
#include <event_data.h>
#include <optional>
#include <serialization_types.h>
#include <string>
#include <traceparser.h>
#include <utility>

namespace ipc::serialization {
using database = absl::flat_hash_map<std::pair<std::string, size_t>,
                                     std::vector<parse::database_elem>>;

class deserializer {
public:
  deserializer(const std::string &socket_path);
  std::optional<std::pair<database, std::vector<size_t>>> read_database();
  ~deserializer();

private:
  std::string read_string();
  common::event_data read_event_data();
  void read_tuple(database &db);
  void read_tuple_list(database &db);
  template<typename T>
  T read_primitive() {
    T t{};
    std::array<char, sizeof(T)> buf;
    boost::asio::read(sock_, boost::asio::buffer(buf));
    std::memcpy(&t, buf.data(), sizeof(T));
    return t;
  }

  std::string path_;
  boost::asio::io_context ctx_;
  boost::asio::local::stream_protocol::acceptor acceptor_;
  boost::asio::buffered_read_stream<boost::asio::local::stream_protocol::socket>
    sock_;
};
}// namespace ipc::serialization

#endif// CPPMON_DESERIALIZATION_H
