%skeleton "lalr1.cc" // -*- C++ -*-
%require "3.7.4"
%defines

%define api.token.raw

%define api.token.constructor
%define api.value.type variant
%define parse.assert

%code requires {
  #include <string>
  #include <formula.h>
  #include <memory.h>
  #include <utility>
  #include <vector>
  #include <stdexcept>
  using namespace fo;
  using std::move;
  using std::string;
  using std::vector;
  class driver;
  void fail(const std::string &m);
  string strip(string s);
}

// The parsing context.
%param { driver& drv }

%locations

%define parse.trace
%define parse.error detailed
%define parse.lac full

%code {
#include "driver.h"
}

%define api.token.prefix {TOK_}
%token AT LPA RPA LCB RCB COM TR
%token <string> STR
%token EOF

%start signature
%type <(Db.schema)> signature

%start tsdb
%type <(Helper.parser_feed)> tsdb

%%


signature:
  predicate signature     { f "signature(list)"; update_preds ($1::$2) }
|                         { f "signature(end)"; update_preds [] }

predicate:
  STR LPA fields RPA      { f "predicate"; make_predicate $1 $3 }




tsdb:
  AT STR db TR            { f "tsdb(next)";   DataTuple { ts = MFOTL.ts_of_string "Log_parser" $2; db = make_db $3; complete = true } }


db:
  table db                { f "db(list)"; add_table $2 $1 }
|                         { f "db()"; [] }

table:
  STR relation            { f "table";
                                  try
                                    make_table $1 $2
                                  with (Failure str) as e ->
                                    if !Misc.ignore_parse_errors then
                                      begin
                                        prerr_endline str;
                                        raise Parsing.Parse_error
                                      end
                                    else
                                      raise e
                                }

relation:
  tuple relation          { f "relation(list)"; $1::$2 }
|                         { f "relation(end)"; [] }

tuple:
LPA fields RPA          { f "tuple"; $2 }


fields:
  STR COM fields          { f "fields(list)"; $1::$3 }
| STR                     { f "fields(end)"; [$1] }
| %empty                         { f "fields()"; [] }

%%

string strip(string s) {
  size_t s_len = s.size();
  if (s[0] == '\"' && s[s_len-1] == '\"') {
    return s.substr(1, s_len-2);
  }
  return s;
}

void fail(const std::string &m) {
  throw std::runtime_error(m);
}

void yy::parser::error (const location_type &, const std::string &m) {
  throw std::runtime_error(m);
}