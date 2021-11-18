#include <deserialization.h>
#include <unistd.h>
#include <util.h>

namespace ipc::serialization {
deserializer::deserializer(const std::string &socket_path)
    : path_(socket_path), acceptor_(ctx_), sock_(ctx_, 5000) {
  ::unlink(path_.c_str());
  acceptor_.open();
  acceptor_.bind(path_.c_str());
  acceptor_.listen();
  acceptor_.accept(sock_.lowest_layer());
}

deserializer::~deserializer() { ::unlink(path_.c_str()); }

std::string deserializer::read_string() {
  auto str_len = read_primitive<size_t>();
  std::string res;
  res.insert(0, str_len, ' ');
  boost::asio::read(sock_, boost::asio::buffer(res.data(), str_len));
  return res;
}

common::event_data deserializer::read_event_data() {
  auto ev_ty = read_primitive<c_ev_ty>();
  if (ev_ty == TY_INT)
    return common::event_data::Int(read_primitive<int64_t>());
  else if (ev_ty == TY_FLOAT)
    return common::event_data::Float(read_primitive<double>());
  else if (ev_ty == TY_STRING)
    return common::event_data::String(read_string());
  else
    throw std::runtime_error("invalid type, expected int|float|string");
}

void deserializer::read_tuple(database &db) {
  auto event_name = read_string();
  auto arity = read_primitive<size_t>();
  parse::database_tuple event;
  event.reserve(arity);
  for (size_t i = 0; i < arity; ++i)
    event.push_back(read_event_data());
  auto key = std::pair(event_name, arity);
  auto it = db.find(key);
  if (it == db.end())
    db.emplace(key, make_vector(make_vector(std::move(event))));
  else
    it->second[0].push_back(std::move(event));
}

void deserializer::read_tuple_list(database &db) {
  control_bits nxt_ctrl;
  while ((nxt_ctrl = read_primitive<control_bits>()) == CTRL_NEW_EVENT)
    read_tuple(db);
  if (nxt_ctrl != CTRL_END_DATABASE)
    throw std::runtime_error("expected end of database");
}

std::optional<std::pair<database, std::vector<size_t>>>
deserializer::read_database() {
  auto nxt_ctrl = read_primitive<control_bits>();
  if (nxt_ctrl == CTRL_TERMINATED) {
    return {};
  } else if (nxt_ctrl == CTRL_NEW_DATABASE) {
    database db;
    auto ts = read_primitive<size_t>();
    read_tuple_list(db);
    return {std::pair(std::move(db), make_vector(ts))};
  } else {
    throw std::runtime_error("expected database");
  }
}
}// namespace ipc::serialization