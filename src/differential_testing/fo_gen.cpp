#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/random/random.h>
#include <fmt/format.h>
#include <fmt/os.h>
#include <fmt/ranges.h>
#include <iterator>
#include <set>
#include <string>
#include <vector>

ABSL_FLAG(bool, IS_BIN_OP, false, "binary operator");
ABSL_FLAG(bool, IS_NEG, false, "negated");
ABSL_FLAG(size_t, EV_RATE, 1, "event rate");
ABSL_FLAG(bool, IS_FUTURE_OP, false, "future operator");
ABSL_FLAG(size_t, TRACE_LEN, 20000, "number of tps");
ABSL_FLAG(size_t, COLLAPSE_TP, 1, "number of tps to collapse");
ABSL_FLAG(std::string, FORMULA_NAME, "output.mfotl",
          "formula output file name");
ABSL_FLAG(std::string, TRACE_NAME, "output.log", "trace output name");

constexpr std::int64_t groups = 20;
constexpr size_t INTVL_LOWER = 10;
constexpr size_t INTVL_UPPER = 20;

void write_formula(bool bin_op, bool neg, bool future) {
  auto out_f = fmt::output_file(absl::GetFlag(FLAGS_FORMULA_NAME));
  if (bin_op)
    out_f.print("q(x, y) AND ({} s(x) {}[{},{}] r(x, y))\n", (neg ? "NOT" : ""),
                (future ? "UNTIL" : "SINCE"), INTVL_LOWER, INTVL_UPPER);
  else
    out_f.print("q(x, y) AND ({}[{},{}] r(x, y))\n",
                (future ? "EVENTUALLY" : "ONCE"), INTVL_LOWER, INTVL_UPPER);
}

using string_v = std::vector<std::string>;
using string_vv = std::vector<string_v>;

string_vv generate_trace(bool bin_op, bool neg, size_t len, size_t er,
                         size_t collapse) {
  using PII = std::pair<int64_t, int64_t>;
  using VPII = std::vector<PII>;
  using VVPII = std::vector<VPII>;
  absl::BitGen gen;
  string_vv lines;
  lines.reserve(len / collapse);
  VVPII r_x_y;
  r_x_y.reserve(len / collapse);
  std::set<std::int64_t> s_x;
  const std::int64_t len_i = static_cast<int64_t>(len / collapse);

  for (size_t i = 0; i < (len / collapse); i++) {
    VPII r_x_y_tmp;
    r_x_y_tmp.reserve(collapse);
    string_v lines_tmp;
    lines_tmp.reserve(collapse);
    std::set<std::int64_t> s_x_tmp;
    for (size_t j = 0; j < collapse; ++j) {
      std::string line;
      std::int64_t rx = (bin_op && !neg) ? absl::Uniform(gen, 0L, groups)
                                         : absl::Uniform(gen, 0L, len_i);
      std::int64_t ry = absl::Uniform(gen, 0L, len_i);
      r_x_y_tmp.push_back(std::pair(rx, ry));
      s_x_tmp.insert(rx);
      line += fmt::format("r({},{})", rx, ry);
      if (bin_op) {
        if (neg) {
          int64_t sx;
          if (absl::Bernoulli(gen, 0.5) && !r_x_y.empty()) {
            size_t idx = absl::Uniform(gen, 0UL, r_x_y.size());
            size_t idx2 = absl::Uniform(gen, 0UL, r_x_y[idx].size());
            assert(!r_x_y[idx].empty());
            sx = r_x_y[idx][idx2].first;
          } else {
            sx = absl::Uniform(gen, 0L, len_i);
          }
          line += fmt::format(" s({})", sx);
        } else if (j == 0) {
          for (auto &it : s_x) {
            if (absl::Uniform(gen, 0L, len_i) == 0L)
              continue;
            line += fmt::format(" s({})", it);
          }
        }
      }
      size_t curr_ts = i / er;
      size_t qfrom = (curr_ts > INTVL_UPPER) ? (curr_ts - INTVL_UPPER) : 0UL;
      assert(INTVL_LOWER >= 1);
      size_t qto =
        (curr_ts > (INTVL_LOWER - 1)) ? (curr_ts - (INTVL_LOWER - 1)) : 0UL;
      qfrom *= er;
      qto *= er;
      std::int64_t qx, qy;
      if (qfrom != qto && absl::Bernoulli(gen, 0.5)) {
        size_t idx = absl::Uniform(gen, qfrom, qto);
        size_t idx2 = absl::Uniform(gen, 0UL, r_x_y[idx].size());
        qx = r_x_y[idx][idx2].first;
        qy = r_x_y[idx][idx2].second;
      } else {
        qx = absl::Uniform(gen, 0L, len_i);
        qy = absl::Uniform(gen, 0L, len_i);
      }
      line += fmt::format(" q({},{})", qx, qy);
      lines_tmp.push_back(line);
    }
    lines.emplace_back(std::move(lines_tmp));
    r_x_y.emplace_back(std::move(r_x_y_tmp));
    s_x.insert(s_x_tmp.cbegin(), s_x_tmp.cend());
  }
  return lines;
}

template<typename Iterator>
void write_trace_helper(Iterator begin, Iterator end, size_t er) {
  auto o_file = fmt::output_file(absl::GetFlag(FLAGS_TRACE_NAME));
  for (size_t i = 0; begin != end; ++begin, ++i) {
    o_file.print("@{} {};\n", i / er, fmt::join(*begin, " "));
  }
}

void write_trace(const string_vv &trace, size_t er, bool future) {
  if (future)
    write_trace_helper(trace.crbegin(), trace.crend(), er);
  else
    write_trace_helper(trace.cbegin(), trace.cend(), er);
}

int main(int argc, char **argv) {
  absl::ParseCommandLine(argc, argv);
  bool bin_op = absl::GetFlag(FLAGS_IS_BIN_OP),
       neg = absl::GetFlag(FLAGS_IS_NEG),
       future = absl::GetFlag(FLAGS_IS_FUTURE_OP);
  size_t er = absl::GetFlag(FLAGS_EV_RATE),
         len = absl::GetFlag(FLAGS_TRACE_LEN),
         collapse = absl::GetFlag(FLAGS_COLLAPSE_TP);
  write_formula(bin_op, neg, future);
  auto trace = generate_trace(bin_op, neg, len, er, collapse);
  write_trace(trace, er, future);
  return 0;
}
