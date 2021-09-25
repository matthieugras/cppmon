#include <traceparser.h>
#include <fmt/core.h>
#include <fmt/ranges.h>
#include <fstream>
#include <string>

int main() {
  using namespace parse;
  std::string_view inp_str = R"kkk(A(int,int)
B(int,int)
C(int,int))kkk";
  trace_parser bla(signature_parser::parse(inp_str));
  std::ifstream infile("/home/rofl/Scratch/sem_proj/triangle_trace_monpoly");
  std::string line;
  while (std::getline(infile, line)) {
    bla.parse_database(line);
  }

  //trace_parser bla(std::move(expted));
}