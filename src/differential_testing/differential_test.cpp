#include <absl/time/civil_time.h>
#include <absl/time/clock.h>
#include <absl/time/time.h>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/process.hpp>
#include <cstdio>
#include <differential_test.h>
#include <fmt/format.h>
#include <future>
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
  while (true) {
    if (bp::system(bp::start_dir(wdir), gen_fma_, "-output", "bla", "-size",
                   "2", bp::std_out > bp::null, bp::std_err > stderr) != 0) {
      throw std::runtime_error("failed to generate signature/formula pair");
    }
    int ret =
      bp::system(bp::start_dir(wdir), monpoly_, "-verified", "-dump_verified",
                 "-sig", "bla.sig", "-formula", "bla.mfotl",
                 bp::std_err > stderr, bp::std_out > (wdir / "bla.json"));
    if (!ret)
      break;
  }
}

void differential_test::gen_trace(const fs::path &wdir) {
  if (bp::system(bp::start_dir(wdir), trace_gen_, "10", "-e", "10", "-i", "5",
                 "-sig", "bla.sig", "-dom", "42",
                 bp::std_out > (wdir / "raw_trace"), bp::std_err > stderr) != 0)
    throw std::runtime_error("failed to generate trace");
  if (bp::system(bp::start_dir(wdir), replayer_, "-a", "0", "-f", "monpoly",
                 "raw_trace", bp::std_err > stderr,
                 bp::std_out > (wdir / "bla.log")) != 0)
    throw std::runtime_error("failed to replay trace");
}

bool differential_test::run_monitors(const fs::path &wdir) {
  /*bp::group mons;
  boost::asio::io_context ios;
  std::future<std::string> cppmon_stdout, monpoly_stdout;
  bp::child monpoly_p(bp::start_dir(wdir), monpoly_, "-verified", "-sig",
                      "bla.sig", "-formula", "bla.mfotl", "-log", "bla.log",
                      bp::std_out > (wdir / "monpoly_out"),
                      bp::std_err > stderr, ios),
    cppmon_p(bp::start_dir(wdir), cppmon_, "--sig", "bla.sig", "--formula",
             "bla.json", "--log", "bla.log",
             bp::std_out > (wdir / "cppmon_out"), bp::std_err > stderr, ios);
  ios.run();
  assert(!cppmon_p.running() && !monpoly_p.running());
  if (cppmon_p.exit_code()) {
    fmt::print(stderr, "cppmon non-zero exit code\n");
    return false;
  }
  if (monpoly_p.exit_code()) {
    fmt::print(stderr, "monpoly non-zero exit code\n");
    return false;
  }
  int differ =
    bp::system(bp::start_dir(wdir), bp::search_path("diff"), "monpoly_out",
               "cppmon_out", bp::std_err > bp::null, bp::std_out > bp::null);
  if (differ)
    return false;
  else
    return true;*/
  int monpoly_ret =
    bp::system(bp::start_dir(wdir), monpoly_, "-verified", "-sig", "bla.sig",
               "-formula", "bla.mfotl", "-log", "bla.log",
               bp::std_out > (wdir / "monpoly_out"), bp::std_err > stderr);
  if (monpoly_ret) {
    fmt::print(stderr, "monpoly returned non-zero exit code");
    return false;
  }
  int cppmon_ret =
    bp::system(bp::start_dir(wdir), cppmon_, "--sig", "bla.sig", "--formula",
               "bla.json", "--log", "bla.log",
               bp::std_out > (wdir / "cppmon_out"), bp::std_err > stderr);
  if (cppmon_ret) {
    fmt::print(stderr, "cppmon returned non-zero exit code\n");
    return false;
  }
  int differ =
    bp::system(bp::start_dir(wdir), bp::search_path("diff"), "monpoly_out",
               "cppmon_out", bp::std_err > stderr, bp::std_out > bp::null);
  if (differ)
    return false;
  else
    return true;
}

void differential_test::run_single_test(size_t iter_num) {
  fs::path wdir = working_dir();
  if (!fs::create_directory(wdir))
    throw std::runtime_error("failed to create working directory");
  gen_sig_formula(wdir);
  gen_trace(wdir);
  if (!run_monitors(wdir)) {
    fmt::print(stderr,
               "test iteration {} failed. saved outputs in folder: \"{}\" for "
               "inspection\n",
               iter_num, wdir.string());
  } else {
    // fmt::print("matching outputs at test iteration {}\n", iter_num);
    // fmt::print(stderr, "removing path {}\n", wdir.string());
    fs::remove_all(wdir);
  }
}

[[noreturn]] void differential_test::run_tests() {
  for (size_t iter_num = 0;; ++iter_num)
    run_single_test(iter_num);
}
}// namespace testing
