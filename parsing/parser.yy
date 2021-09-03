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
  using std::make_unique;
  using std::string;
  using std::unique_ptr;
  using std::vector;
  class driver;
  struct tu_pair {
    int ticks;
    char spec;
  };
  template <class T, class... Args>
  unique_ptr<T> mk_p(Args &&... args) {
    return make_unique<T>(std::forward<Args...>(args...));
  }
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
%token FALSE TRUE
%token LPA RPA LSB RSB COM SC DOT QM LD LESSEQ EQ LESS GTR GTREQ STAR LARROW
%token PLUS MINUS SLASH MOD F2I I2F
%token <string> STR STR_CST
%token <double> INT RAT
%token <tu_pair> TU
%token LET IN NOT AND OR IMPL EQUIV EX FA
%token PREV NEXT EVENTUALLY ONCE ALWAYS PAST_ALWAYS SINCE UNTIL BAR FREX PREX
%token CNT MIN MAX SUM AVG MED
%token END 0 "end of file"

%right SINCE UNTIL
%nonassoc PREV NEXT EVENTUALLY ONCE ALWAYS PAST_ALWAYS 
%nonassoc EX FA
%left EQUIV
%right IMPL
%left OR
%left AND
%left ALT
%left CONCAT
%nonassoc LPA
%nonassoc NOT
%nonassoc BASE
%nonassoc QM
%nonassoc RPA


%left PLUS MINUS          /* lowest precedence */
%left STAR DIV            /* medium precedence */
%nonassoc UMINUS F2I I2F  /* highest precedence */

%start formula
%type <formula> formula
%type <string> var
%type <vector<string>> varlist
%type <cst_val> cst
%type <term> term
%type <vector<term>> termlist;

%%

formula:
  FALSE { 
    $$ = formula(Equal({
      mk_p<term>(Cst(CInt(0))),
      mk_p<term>(Cst(CInt(1)))
    }));
  }
| TRUE {
    $$ = formula(Equal({
      mk_p<term>(Cst(CInt(0))),
      mk_p<term>(Cst(CInt(0)))
    }));
  }

term:
  term PLUS term          { $$ = term(Plus({mk_p<term>(move($1)), mk_p<term>(move($3)}))); }
| term MINUS term         { $$ = term(Minus({mk_p<term>(move($1)), mk_p<term>(move($3)}))); }
| term STAR term          { $$ = term(Mult({mk_p<term>(move($1)), mk_p<term>(move($3)}))); }
| term SLASH term         { $$ = term(Div({mk_p<term>(move($1)), mk_p<term>(move($3)}))); }
| term MOD term           { $$ = term(Mod({mk_p<term>(move($1)), mk_p<term>(move($3)}))); }
| MINUS term %prec UMINUS { $$ = term(UMinus(mk_p<term>(move($2)))); }
| LPA term RPA            { $$ = move($2); }
| F2I LPA term RPA        { $$ = term(F2i(mk_p<term>(move($3)))); }
| I2F LPA term RPA        { $$ = term(I2f(mk_p<term>(move($3)))); }
| cst                     { $$ = term(Cst(move($1))); }
| var                     { $$ = term(Var(move($1))); }

cst:
  INT                     { CInt($1) }
| RAT                     { CFloat($1) }
| STR_CST                 { CStr(strip(move($1))) }

termlist:
  term COM termlist       { $1 :: $3 }
| term                    { $$ = vector<term>{move($1)}; }
|                         { $$ = vector<term>(); }

varlist:
  varlist COM var         { $$ = move($1); $$.emplace_back(move($3)); }
| var                     { $$ = vector<string>{move($1)}; }
|                         { $$ = vector<string>() }

var:
  LD    { drv.var_cnt++; $$ = string("_") + std::to_string(drv.var_cnt); }
| STR   { string var_name($1);
          if (var_name.size() < 1) {
            fail("variable has incorrect size");
          }
          $$ = var_name; }

  

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