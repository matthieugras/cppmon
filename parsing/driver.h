#ifndef DRIVER_H
#define DRIVER_H
#include <string>
#include <parser.h>

# define YY_DECL \
  yy::parser::symbol_type yylex (driver& drv)
YY_DECL;

class driver
{
public:
  size_t var_cnt = 0;
  driver ();
  int result;
  int parse (const std::string& f);
  std::string file;
  bool trace_parsing;
  void scan_begin ();
  void scan_end ();
  bool trace_scanning;
  yy::location location;
};
#endif