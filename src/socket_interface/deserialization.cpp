#include <array>
#include <deserialization.h>
#include <unistd.h>
#include <util.h>

namespace ipc::serialization {
deserializer::deserializer(std::string socket_path, pred_map_t pred_map)
    : path_(std::move(socket_path)), acceptor_(ctx_), sock_(ctx_, 5000),
      pred_map_(std::move(pred_map)) {
  ::unlink(path_.c_str());
  acceptor_.open();
  acceptor_.bind(path_.c_str());
  acceptor_.listen();
  acceptor_.accept(sock_.lowest_layer());
}

deserializer::~deserializer() { ::unlink(path_.c_str()); }

std::string deserializer::read_string() {
  // fmt::print("reading string\n");
  auto str_len = read_primitive<size_t>();
  // fmt::print("string length is {}\n", str_len);
  std::string res;
  res.insert(0, str_len, ' ');
  boost::asio::read(sock_, boost::asio::buffer(res.data(), str_len));
  return res;
}

common::event_data deserializer::read_event_data() {
  auto ev_ty = read_primitive<c_ev_ty>();
  if (ev_ty == TY_INT) {
    // fmt::print("reading int event data\n");
    return common::event_data::Int(read_primitive<int64_t>());
  } else if (ev_ty == TY_FLOAT) {
    // fmt::print("reading float event data\n");
    return common::event_data::Float(read_primitive<double>());
  } else if (ev_ty == TY_STRING) {
    // fmt::print("reading string event data\n");
    return common::event_data::String(read_string());
  } else
    throw std::runtime_error("invalid type, expected int|float|string");
}

void deserializer::read_tuple(database &db) {
  // fmt::print("reading event name\n");
  auto event_name = read_string();
  // fmt::print("event name is: {}\n", event_name);
  auto arity = read_primitive<size_t>();
  // fmt::print("arity of event is {}\n", arity);
  auto p_key = std::pair(std::move(event_name), arity);
  auto p_it = pred_map_.find(p_key);
  auto pred_id = p_it->second;
  if (p_it == pred_map_.end()) {
    // fmt::print("skipping event\n");
    for (size_t i = 0; i < arity; ++i)
      read_event_data();
  } else {
    parse::database_tuple event;
    event.reserve(arity);
    for (size_t i = 0; i < arity; ++i)
      event.push_back(read_event_data());
    auto it = db.find(pred_id);
    if (it == db.end())
      db.emplace(pred_id, make_vector(make_vector(std::move(event))));
    else
      it->second[0].push_back(std::move(event));
  }
}

void deserializer::read_tuple_list(database &db) {
  control_bits nxt_ctrl;
  while ((nxt_ctrl = read_primitive<control_bits>()) == CTRL_NEW_EVENT) {
    // fmt::print("reading new event\n");
    read_tuple(db);
  }
  // fmt::print("finished reading event\n");
  if (nxt_ctrl != CTRL_END_DATABASE) {
    // fmt::print("expected database is: {}\n", nxt_ctrl);
    throw std::runtime_error("expected end of database");
  }
}

void deserializer::send_latency_marker(int64_t wm) {
  // fmt::print("sending latency marker {}\n", wm);
  send_primitive(CTRL_LATENCY_MARKER);
  send_primitive(wm);
}

void deserializer::send_eof() {
  send_primitive(CTRL_EOF);
  sock_.next_layer().shutdown(boost::asio::socket_base::shutdown_send);
}

deserializer::record_type deserializer::read_database(ts_database &ts_db,
                                                      int64_t &wm) {
  auto nxt_ctrl = read_primitive<control_bits>();
  if (nxt_ctrl == CTRL_EOF) {
    // fmt::print("read CMD eof\n");
    return EOF_RECORD;
  } else if (nxt_ctrl == CTRL_NEW_DATABASE) {
    // fmt::print("read CMD new db\n");
    auto ts = read_primitive<size_t>();
    // fmt::print("db has ts {}\n", ts);
    read_tuple_list(ts_db.first);
    ts_db.second.push_back(ts);
    return DB_RECORD;
  } else if (nxt_ctrl == CTRL_LATENCY_MARKER) {
    // fmt::print("read CMD latency marker\n");
    wm = read_primitive<int64_t>();
    // fmt::print("latency marker is {}\n", wm);
    return LATENCY_RECORD;
  } else {
    throw std::runtime_error("expected database");
  }
}
}// namespace ipc::serialization
