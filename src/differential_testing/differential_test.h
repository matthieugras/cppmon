#ifndef CPPMON_DIFFERENTIAL_TEST_H
#define CPPMON_DIFFERENTIAL_TEST_H

#include <boost/filesystem.hpp>

namespace testing {
class differential_test {
public:
  differential_test(boost::filesystem::path cppmon,
                    boost::filesystem::path monpoly,
                    boost::filesystem::path gen_fma,
                    boost::filesystem::path trace_gen,
                    boost::filesystem::path replayer);
  [[noreturn]] void run_tests();

private:
  void run_single_test(size_t iter_num);
  void gen_sig_formula(const boost::filesystem::path &wdir);
  void gen_trace(const boost::filesystem::path &wdir);
  bool run_monitors(const boost::filesystem::path &wdir);
  boost::filesystem::path cppmon_, monpoly_, gen_fma_, trace_gen_, replayer_;
};
}// namespace testing

#endif// CPPMON_DIFFERENTIAL_TEST_H
