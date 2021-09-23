#ifndef DRIVER_H
#define DRIVER_H
#include <parser.h>
#include <string>
#include <vector>

#define YY_DECL yy::parser::symbol_type yylex(driver &drv)
YY_DECL;

enum class sig_t {
  int_type,
  float_type,
  string_type
};

class driver {
public:
  size_t var_cnt = 0;
  driver();
  int result;
  int parse(const std::string &f);
  std::string file;
  bool trace_parsing;
  void scan_begin();
  void scan_end();
  bool trace_scanning;
  yy::location location;
};
#endif