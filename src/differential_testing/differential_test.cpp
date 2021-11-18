#include <absl/time/clock.h>
#include <absl/time/time.h>
#include <boost/asio/post.hpp>
#include <boost/asio/thread_pool.hpp>
#include <boost/process.hpp>
#include <boost/system/error_code.hpp>
#include <cstdio>
#include <differential_test.h>
#include <error.h>
#include <fmt/format.h>
#include <stdexcept>


namespace testing {

namespace {
  namespace fs = boost::filesystem;
  namespace bp = boost::process;
}// namespace

void verify_exists_and_regular(const fs::path &p) {
  if (!fs::exists(p))
    throw std::runtime_error(fmt::format("file {} does not exist", p.string()));
  if (!fs::is_regular_file(p))
    throw std::runtime_error(
      fmt::format("file {} is not a regular file", p.string()));
}

differential_test::differential_test(fs::path cppmon, fs::path monpoly,
                                     fs::path gen_fma, fs::path trace_gen,
                                     fs::path replayer)
    : cppmon_(std::move(cppmon)), monpoly_(std::move(monpoly)),
      gen_fma_(std::move(gen_fma)), trace_gen_(std::move(trace_gen)),
      replayer_(std::move(replayer)) {
  verify_exists_and_regular(cppmon_);
  verify_exists_and_regular(monpoly_);
  verify_exists_and_regular(gen_fma_);
  verify_exists_and_regular(trace_gen_);
  verify_exists_and_regular(replayer_);
}

static fs::path working_dir() {
  for (size_t counter = 0;; ++counter) {
    auto now_as_str =
      fmt::format("{}-{}", absl::FormatTime(absl::Now()), counter);
    if (!fs::exists(now_as_str))
      return now_as_str;
  }
}

void differential_test::gen_sig_formula(const fs::path &wdir) {
  const size_t MAX_ITERS = 200;
  for (size_t i = 0;; ++i) {
    // size == 4, free_vars == 5
    if (bp::system(bp::start_dir(wdir), gen_fma_, "-output", "bla", "-size",
                   "4", "-free_vars", "5", "-max_interval", "20",
                   bp::std_out > bp::null,
                   bp::std_err > "gen_fma_stderr") != 0) {
      throw std::runtime_error("failed to generate signature/formula pair");
    }
    int ret =
      bp::system(bp::start_dir(wdir), monpoly_, "-verified", "-dump_verified",
                 "-sig", "bla.sig", "-formula", "bla.mfotl",
                 bp::std_err > (wdir / "convert_formula_stderr"),
                 bp::std_out > (wdir / "bla.json"));
    if (!ret && !fs::is_empty(wdir / "bla.sig")) {
      break;
    } else {
      boost::system::error_code ec;
      fs::remove(wdir / "bla.json", ec);
    }
    if (i >= MAX_ITERS)
      throw std::runtime_error(fmt::format(
        "failed to generate valid formula after {} iterations", MAX_ITERS));
  }
}

void differential_test::gen_trace(const fs::path &wdir) {
  std::string seed =
    std::to_string(absl::Uniform<size_t>(bitgen_, 0UL, 100000000UL));
  if (bp::system(bp::start_dir(wdir), trace_gen_, "60", "-e", "20", "-i", "2",
                 "-sig", "bla.sig", "-dom", "20", "-seed", seed,
                 bp::std_out > (wdir / "raw_trace"),
                 bp::std_err > (wdir / "trace_gen_stderr")) != 0)
    throw std::runtime_error("failed to generate trace");
  if (bp::system(bp::start_dir(wdir), replayer_, "-a", "0", "-f", "monpoly",
                 "raw_trace", bp::std_err > (wdir / "replayer_stderr"),
                 bp::std_out > (wdir / "bla.log")) != 0)
    throw std::runtime_error("failed to replay trace");
}

bool differential_test::run_monitors(const fs::path &wdir) {
  int monpoly_ret =
    bp::system(bp::start_dir(wdir), monpoly_, /*"-verified",*/ "-sig",
               "bla.sig", "-formula", "bla.mfotl", "-log", "bla.log",
               bp::std_out > (wdir / "monpoly_out"),
               bp::std_err > (wdir / "monpoly_stderr"));
  if (monpoly_ret) {
    fmt::print(stderr, "monpoly returned non-empty exit code\n");
    return false;
  }
  int cppmon_ret = bp::system(bp::start_dir(wdir), cppmon_, "--sig", "bla.sig",
                              "--formula", "bla.json", "--log", "bla.log",
                              bp::std_out > (wdir / "cppmon_out"),
                              bp::std_err > (wdir / "cppmon_stderr"));
  if (cppmon_ret) {
    fmt::print(stderr, "cppmon returned non-zero exit code\n", wdir.string());
    return false;
  }
  int differ = bp::system(
    bp::start_dir(wdir), bp::search_path("diff"), "monpoly_out", "cppmon_out",
    bp::std_err > (wdir / "diff_stderr"), bp::std_out > bp::null);
  if (differ)
    return false;
  else
    return true;
}

void differential_test::run_single_test(size_t iter_num, fs::path wdir) {
  gen_sig_formula(wdir);
  gen_trace(wdir);
  if (!run_monitors(wdir)) {
    fmt::print(stderr,
               "test iteration {} failed. saved outputs in folder: \"{}\" for "
               "inspection\n",
               iter_num, wdir.string());
  } else {
    fs::remove_all(wdir);
  }
  free_slots_->release();
}

void differential_test::run_tests() {
  boost::asio::thread_pool pool(13);
  free_slots_ = std::make_unique<std::counting_semaphore<>>(30);
  for (size_t iter_num = 0; iter_num < 400000; ++iter_num) {
    if (iter_num % 1000 == 0)
      fmt::print("iteration {}\n", iter_num);
    free_slots_->acquire();
    fs::path wdir = working_dir();
    if (!fs::create_directory(wdir))
      throw std::runtime_error("failed to create working directory");
    boost::asio::post(pool, [this, iter_num, wdir = std::move(wdir)]() {
      run_single_test(iter_num, std::move(wdir));
    });
  }
  pool.join();
}
}// namespace testing
