#include <serialization.h>

namespace ipc::serialization {
serializer::serializer(const std::string &socket_path) : wstream_(ctx_, 5000) {
  wstream_.lowest_layer().connect(socket_path);
}

void serializer::send_string(const char *str) {
  size_t len = strlen(str);
  send_primitive(len);
  boost::asio::write(wstream_, boost::asio::const_buffer(str, len));
}

void serializer::send_event_data(c_ev_ty ty, const c_ev_data &data) {
  send_primitive(ty);
  if (ty == TY_INT)
    send_primitive(data.i);
  else if (ty == TY_FLOAT)
    send_primitive(data.d);
  else if (ty == TY_STRING)
    send_string(data.s);
  else
    throw std::runtime_error("invalid type, expected int|float|string");
}

void serializer::send_event(const char *name, const c_ev_ty *tys,
                            const c_ev_data *data, size_t arity) {
  send_primitive(CTRL_NEW_EVENT);
  send_string(name);
  send_primitive(arity);
  for (size_t i = 0; i < arity; ++i)
    send_event_data(tys[i], data[i]);
}

void serializer::send_end_db() { send_primitive(CTRL_END_DATABASE); }

void serializer::send_begin_db(size_t ts) {
  send_primitive(CTRL_NEW_DATABASE);
  send_primitive(ts);
}

void serializer::send_terminate() {
  send_primitive(CTRL_TERMINATED);
  wstream_.flush();
}
}// namespace ipc::serialization