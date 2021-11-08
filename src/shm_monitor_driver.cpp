#include "shm_monitor_driver.h"

namespace {
namespace boost_ipc = ipc::boost_ipc;
}

static common::event_data
event_data_from_buf(const ipc::event_data &ev_dat_in) {
  auto fn = [](auto &&arg) -> common::event_data {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (std::is_same_v<T, int>) {
      return common::event_data::Int(arg);
    } else if constexpr (std::is_same_v<T, double>) {
      return common::event_data::Float(arg);
    } else {
      static_assert(std::is_same_v<T, ipc::string>, "type missmatch");
      return common::event_data::String(std::string(arg));
    }
  };
  return boost::variant2::visit(fn, ev_dat_in);
}


static parse::database_tuple tup_from_buf(const ipc::event_data_list &in_tup) {
  parse::database_tuple out_tup;
  out_tup.reserve(in_tup.size());
  for (const auto &ev_data_in : in_tup) {
    out_tup.push_back(event_data_from_buf(ev_data_in));
  }
  return out_tup;
}

static std::pair<monitor::database, monitor::ts_list>
db_from_buf(const ipc::database_buffer &buf) {
  monitor::database res_db;
  size_t num_dbs = buf.size();
  auto it = buf.begin();
  std::vector<size_t> ts;
  ts.reserve(num_dbs);
  for (size_t i = 0; i < num_dbs; ++i, ++it) {
    ts.push_back(it->first);
    for (auto &[shm_name, data] : it->second) {
      size_t arity = data.size();
      std::string name(shm_name);
      std::pair<std::string, size_t> key(name, arity);
      auto res_it = res_db.find(key);
      auto tup = tup_from_buf(data);
      if (res_it == res_db.end()) {
        std::vector<parse::database_elem> new_ent(num_dbs);
        new_ent[i].push_back(std::move(tup));
      } else {
        res_it->second[i].push_back(std::move(tup));
      }
    }
  }
  return {std::move(res_db), std::move(ts)};
}

shm_monitor_driver::shm_monitor_driver(
  const std::filesystem::path &formula_path,
  const std::filesystem::path &sig_path, std::string shm_id, size_t shm_size)
    : shm_id_(std::move(shm_id)), ipc_state_(nullptr) {
  auto formula = fo::Formula(read_file(formula_path));
  sig_ = parse::signature_parser::parse(read_file(sig_path));
  monitor_ = monitor::monitor(formula);
  setup_shared_memory(shm_size);
}

shm_monitor_driver::~shm_monitor_driver() noexcept {}

void shm_monitor_driver::setup_shared_memory(size_t shm_size) {
  boost_ipc::shared_memory_object::remove(shm_id_.c_str());
  segment_ = boost_ipc::managed_shared_memory(boost_ipc::create_only,
                                              shm_id_.c_str(), shm_size);
  ipc::void_allocator alloc_inst(segment_.get_segment_manager());
  segment_.construct<ipc::shared_state>(boost_ipc::unique_instance)(alloc_inst);
}

void shm_monitor_driver::do_monitor() {
  while (true) {
    auto [buf, done] = ipc_state_->get_events();
    auto [db, ts] = db_from_buf(buf);
    auto sats = monitor_.step(db, ts);
    print_satisfactions(sats);
    if (done) {
      sats = monitor_.last_step();
      print_satisfactions(sats);
      return;
    }
  }
}
