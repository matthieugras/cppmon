#include <serialization.h>

namespace ipc::serialization {
serializer::serializer(const std::string &socket_path, bool unbounded_buffering)
    : wstream_(ctx_, 5000), sock_(ctx_), unbounded_buf_(unbounded_buffering),
      has_data_(false), is_done_(false), should_read_(false) {
  if (unbounded_buf_)
    sock_.connect(socket_path);
  else
    wstream_.lowest_layer().connect(socket_path);
  if (unbounded_buf_)
    worker_.emplace(std::thread(&serializer::worker_fun, this));
}

void serializer::worker_fun() {
  size_t bla = 0;
  while (true) {
    bla++;
    std::unique_lock lk(buf_mut_);
    while (!should_read_ && !is_done_)
      cond_.wait(lk);
    bool done_saved = is_done_;
    if (should_read_ || (done_saved && has_data_)) {
      size_t n_avail = buf_.size();
      std::vector<char> buf_cp(n_avail);
      boost::asio::buffer_copy(boost::asio::buffer(buf_cp), buf_.data());
      buf_.consume(n_avail);
      should_read_ = false;
      has_data_ = false;
      if (!is_done_)
        lk.unlock();
      boost::asio::write(sock_, boost::asio::buffer(buf_cp));
    }
    if (done_saved) {
      sock_.shutdown(boost::asio::socket_base::shutdown_send);
      return;
    }
  }
}

void serializer::send_to_buffer(const char *data, size_t len) {
  std::unique_lock lk(buf_mut_);
  auto new_buf = buf_.prepare(len);
  boost::asio::buffer_copy(new_buf, boost::asio::const_buffer(data, len));
  buf_.commit(len);
  has_data_ = true;
  if (!should_read_ && buf_.size() > 10000) {
    should_read_ = true;
    cond_.notify_one();
  }
}

void serializer::send_string(const char *str) {
  size_t len = strlen(str);
  send_primitive(len);
  if (unbounded_buf_)
    send_to_buffer(str, len);
  else
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
  if (unbounded_buf_) {
    std::unique_lock lk(buf_mut_);
    is_done_ = true;
    cond_.notify_one();
    lk.unlock();
    worker_->join();
  } else {
    wstream_.flush();
    wstream_.lowest_layer().shutdown(boost::asio::socket_base::shutdown_send);
  }
}
}// namespace ipc::serialization