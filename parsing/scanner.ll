%{ /* -*- C++ -*- */
# include <cerrno>
# include <climits>
# include <cstdlib>
# include <cstring> // strerror
# include <string>
# include "driver.h"
# include "parser.h"
using std::string;
%}

%option noyywrap nounput noinput batch debug

%{
  yy::parser::symbol_type make_TU(const string &s, const yy::parser::location_type& loc);
  yy::parser::symbol_type make_INT(const string &s, const yy::parser::location_type& loc);
  yy::parser::symbol_type make_RAT(const string &s, const yy::parser::location_type& loc);
  void f(const string &arg);
%}

lc [a-z]
uc [A-Z]
letter {uc}|{lc}
digit [0-9]
integer ("-")?{digit}+
rational ("-")?{digit}+"."{digit}*
unit {digit}+{letter}
any_string ({letter}|{digit}|"_"|"-"|"/"|":"|"\'"|"\"")*
string ({letter}|{digit}|"_"){any_string}
str_cst ("\'"{any_string}"\'")|("\""{any_string}"\"")|("\'""["{any_string}"]""\'")|("\"""["{any_string}"]""\"")
line_comment "#"[^{}\n\r]*(\n|\r)
multi_line_comment "(*"[^{}]*"*)"

%{
  # define YY_USER_ACTION  loc.columns (yyleng);
%}
%%
%{
  yy::location &loc = drv.location;
  loc.step();
%}


"."                         { f("DOT"); return yy::parser::make_DOT(loc); }
"*"                         { f("STAR"); return yy::parser::make_STAR(loc); }
"/"                         { f("SLASH"); return yy::parser::make_SLASH(loc); }
"("                         { f("LPA"); return yy::parser::make_LPA(loc); }
")"                         { f("RPA"); return yy::parser::make_RPA(loc); }
"["                         { f("LSB"); return yy::parser::make_LSB(loc); }
"]"                         { f("RSB"); return yy::parser::make_RSB(loc); }
"|>"|"▷"                    { }
"FORWARD"|"MATCHF"          { f("FREX"); return yy::parser::make_FREX(loc); }
"<|"|"◁"                    { }
"BACKWARD"|"MATCHP"         { f("PREX"); return yy::parser::make_PREX(loc); }
"|"                         { f("BAR"); return yy::parser::make_BAR(loc); }
","                         { f("COM"); return yy::parser::make_COM(loc); }
";"                         { f("SC"); return yy::parser::make_SC(loc); }
"?"                         { f("QM"); return yy::parser::make_QM(loc); }
"_"                         { f("LD"); return yy::parser::make_LD(loc); }
"<-"                        { f("LARROW"); return yy::parser::make_LARROW(loc); }
"<="                        { f("LESSEQ"); return yy::parser::make_LESSEQ(loc); }
"<"                         { f("LESS"); return yy::parser::make_LESS(loc); }
">="                        { f("GTREQ"); return yy::parser::make_GTREQ(loc); }
">"                         { f("GTR"); return yy::parser::make_GTR(loc); }
"="                         { f("EQ"); return yy::parser::make_EQ(loc); }
"MOD"                       { f("NOT"); return yy::parser::make_NOT(loc); }
"f2i"                       { f("F2I"); return yy::parser::make_F2I(loc); }
"i2f"                       { f("I2F"); return yy::parser::make_I2F(loc); }
"FALSE"                     { f("FALSE"); return yy::parser::make_FALSE(loc); }
"TRUE"                      { f("TRUE"); return yy::parser::make_TRUE(loc); }
"LET"                       { f("LET"); return yy::parser::make_LET(loc); }
"IN"                        { f("IN"); return yy::parser::make_IN(loc); }
"NOT"                       { f("NOT"); return yy::parser::make_NOT(loc); }
"AND"                       { f("AND"); return yy::parser::make_AND(loc); }
"OR"                        { f("OR"); return yy::parser::make_OR(loc); }
"IMPLIES"                   { f("IMPL"); return yy::parser::make_IMPL(loc); }
"EQUIV"                     { f("EQUIV"); return yy::parser::make_EQUIV(loc); }
"EXISTS"                    { f("EX"); return yy::parser::make_EX(loc); }
"FORALL"                    { f("FA"); return yy::parser::make_FA(loc); }
"PREV"                      { f("PREV"); return yy::parser::make_PREV(loc); }
"PREVIOUS"                  { f("PREV"); return yy::parser::make_PREV(loc); }
"NEXT"                      { f("NEXT"); return yy::parser::make_NEXT(loc); }
"EVENTUALLY"                { f("EVENTUALLY"); return yy::parser::make_EVENTUALLY(loc); }
"SOMETIMES"                 { f("EVENTUALLY"); return yy::parser::make_EVENTUALLY(loc); }
"ONCE"                      { f("ONCE"); return yy::parser::make_ONCE(loc); }
"ALWAYS"                    { f("ALWAYS"); return yy::parser::make_ALWAYS(loc); }
"PAST_ALWAYS"               { f("PAST_ALWAYS"); return yy::parser::make_PAST_ALWAYS(loc); }
"HISTORICALLY"              { f("PAST_ALWAYS"); return yy::parser::make_PAST_ALWAYS(loc); }
"SINCE"                     { f("SINCE"); return yy::parser::make_SINCE(loc); }
"UNTIL"                     { f("UNTIL"); return yy::parser::make_UNTIL(loc); }
"CNT"                       { f("CNT"); return yy::parser::make_CNT(loc); }
"MIN"                       { f("MIN"); return yy::parser::make_MIN(loc); }
"MAX"                       { f("MAX"); return yy::parser::make_MAX(loc); }
"SUM"                       { f("SUM"); return yy::parser::make_SUM(loc); }
"AVG"                       { f("AVG"); return yy::parser::make_AVG(loc); }
"MED"                       { f("MED"); return yy::parser::make_MED(loc); }
{unit}                      { f("TU"); return make_TU(yytext, loc); }
{integer}                   { f("INT"); return make_INT(yytext, loc); }
{rational}                  { f("RAT"); return make_RAT(yytext, loc); }
{str_cst}                   { f("STR_CST"); return yy::parser::make_STR_CST(yytext, loc); }
{string}                    { f("STR"); return yy::parser::make_STR(yytext, loc); }
{multi_line_comment}        { // Into the trash it goes
                              f("multi-line comment"); }
{line_comment}              { // Into the trash it goes
                              f("line comment");
                            }
<<EOF>>                     { return yy::parser::make_END(loc); }
%%

yy::parser::symbol_type make_TU(const string &s, const yy::parser::location_type& loc) {
    int ticks = std::stoi(s.substr(0, s.size() - 1));
    char spec = s[s.size() - 1];
    return yy::parser::make_TU(tu_pair{ticks, spec}, loc);
}

yy::parser::symbol_type make_INT(const string &s, const yy::parser::location_type& loc) {
    return yy::parser::make_INT(std::stoi(s), loc);
}

yy::parser::symbol_type make_RAT(const string &s, const yy::parser::location_type& loc) {
    return yy::parser::make_RAT(std::stod(s), loc);
}

void driver::scan_begin () {
  yy_flex_debug = trace_scanning;
  if (file.empty () || file == "-")
    yyin = stdin;
  else if (!(yyin = fopen (file.c_str (), "r")))
    {
      std::cerr << "cannot open " << file << ": " << strerror (errno) << '\n';
      exit (EXIT_FAILURE);
    }
}

void f(const string &arg) {
  std::cout << "<<" << arg << ">>" << yytext << std::endl;
}

void driver::scan_end () {
  fclose (yyin);
}