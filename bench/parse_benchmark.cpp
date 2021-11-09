#include <benchmark/benchmark.h>
#include <string>
#include <traceparser.h>
#include <util.h>

int main() {
  parse::signature sig = parse::signature_parser::parse(read_file("input_sig"));
  parse::trace_parser db_parser(std::move(sig));
  std::ifstream log_file("input_log");
  std::string db_str;
  while (std::getline(log_file, db_str)) {
    benchmark::DoNotOptimize(db_str);
    benchmark::DoNotOptimize(db_parser.parse_database(db_str));
  }
}