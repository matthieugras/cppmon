#include <absl/container/flat_hash_set.h>
#include <absl/flags/flag.h>
#include <absl/flags/parse.h>
#include <absl/flags/usage.h>
#include <absl/random/random.h>
#include <boost/variant2/variant.hpp>
#include <dbgen.h>
#include <event_data.h>
#include <filesystem>
#include <fmt/os.h>
#include <fmt/ranges.h>
#include <formula.h>
#include <monitor.h>
#include <optional>
#include <stdexcept>
#include <string>
#include <traceparser.h>
#include <type_traits>
#include <util.h>
#include <utility>
#include <vector>

ABSL_FLAG(std::string, formula_path, "",
          "filepath for the formula (in json format)");
ABSL_FLAG(std::string, sig_path, "", "filepath for the signature");
ABSL_FLAG(std::string, out_path, "", "output path for the generated database");
ABSL_FLAG(size_t, num_satisfactions, 20,
          "the minimum number of satisfactions to produce");
ABSL_FLAG(size_t, num_tp, 1, "number of time points");

namespace {
namespace var2 = boost::variant2;
using common::event_data;
using fo::Formula;
using monitor::event_table;
using parse::database;
using parse::database_elem;
using parse::signature;
using parse::signature_parser;
using parse::timestamped_database;
using simple_tab = std::vector<std::vector<int64_t>>;
}// namespace

template<typename S>
static S set_intersect(S s1, S s2) {
  if (s1.size() > s2.size())
    std::swap(s1, s2);
  S res;
  for (const auto &elem : s1)
    if (s2.contains(elem))
      res.insert(elem);
  return res;
}

template<typename S>
static S set_union(S s1, S s2) {
  S res = s1;
  res.insert(s2.cbegin(), s2.cend());
  return res;
}

template<typename S, typename T>
static S set_rm_var(S s, T t) {
  S res = s;
  res.erase(t);
  return res;
}

void dbgen::merge_ts_db_list(ts_db_list &ts_dbs1, ts_db_list &ts_dbs2) {
  for (const auto &[ts2, db2] : ts_dbs2) {
    auto &db1 = ts_dbs1[ts2];
    for (const auto &[pred_name, tups2] : db2) {
      auto &tups1 = db1[pred_name];
      tups1.insert(tups1.end(), std::make_move_iterator(tups2.begin()),
                   std::make_move_iterator(tups2.end()));
    }
  }
}

dbgen::sub_form_info dbgen::validate_formula_impl(const Formula &formula,
                                                  const fo::fv_set &all_fvs) {
  using fo::Formula;
  using std::is_same_v;
  namespace var2 = boost::variant2;
  const auto fvs = formula.fvs();
  auto visitor = [&fvs, &all_fvs, this](auto &&arg) -> sub_form_info {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, Formula::eq_t, Formula::less_eq_t,
                                   Formula::less_t, Formula::neg_t,
                                   Formula::since_t, Formula::until_t,
                                   Formula::prev_t>) {
      throw std::runtime_error("unsupported fragment");
    } else if constexpr (any_type_equal_v<T, Formula::next_t>) {
      return validate_formula_impl(*arg.phi, all_fvs);
    } else if constexpr (is_same_v<T, Formula::pred_t>) {
      if (fvs.empty())
        throw std::runtime_error(fmt::format(
          "formula contains closed predicate with name: {}", arg.pred_name));
      return sub_form_info{{arg.pred_name}};
    } else if constexpr (is_same_v<T, Formula::or_t>) {
      auto info_l = validate_formula_impl(*arg.phil, all_fvs),
           info_r = validate_formula_impl(*arg.phir, all_fvs);
      if (!set_intersect(info_l.pred_names, info_r.pred_names).empty())
        throw std::runtime_error("formula contains repeated predicate names");
      return sub_form_info{set_union(info_l.pred_names, info_r.pred_names)};
    } else if constexpr (is_same_v<T, Formula::and_t>) {
      auto info_l = validate_formula_impl(*arg.phil, all_fvs);
      sub_form_info info_r;
      if (const auto *right_rec = arg.phir->inner_if_neg()) {
        info_r = validate_formula_impl(*right_rec, all_fvs);
      } else {
        info_r = validate_formula_impl(*arg.phir, all_fvs);
      }
      if (!set_intersect(info_l.pred_names, info_r.pred_names).empty())
        throw std::runtime_error("formula contains repeated predicate names");
      return sub_form_info{set_union(info_l.pred_names, info_r.pred_names)};
    } else if constexpr (is_same_v<T, Formula::exists_t>) {
      auto child_fvs = arg.phi->fvs();
      if (!child_fvs.contains(0))
        throw std::runtime_error(
          "formula not srnf because bound variable is not used");
      if (set_rm_var(child_fvs, 0UL).empty())
        throw std::runtime_error(
          "formula not closed because the only free variable in one of the "
          "exists is the bound variable");
      auto info_sub = validate_formula_impl(*arg.phi, all_fvs);
      return sub_form_info{info_sub.pred_names};
    } else {
      throw std::runtime_error("unsupported fragment");
    }
  };
  return var2::visit(visitor, formula.val);
}

void dbgen::update_or_insert(database &db, const std::string &pred_name,
                             const std::vector<int64_t> &pred_args) {
  auto it = db.find(pred_name);
  std::vector<event_data> ev_dat;
  ev_dat.reserve(pred_args.size());
  for (const auto d : pred_args)
    ev_dat.push_back(event_data::Int(d));
  if (it == db.end()) {
    auto new_db_elem = database_elem();
    new_db_elem.push_back(std::move(ev_dat));
    db.insert(std::pair(pred_name, std::move(new_db_elem)));
  } else {
    it->second.push_back(std::move(ev_dat));
  }
}
dbgen::simple_tab dbgen::prepend_col(const simple_tab &tab,
                                     const std::vector<int64_t> &col_vals) {
  simple_tab res_tab;
  for (const auto &row : tab) {
    std::vector<int64_t> new_row(row.size() + 1);
    new_row[0] = col_vals[0];
    for (size_t i = 1; i < row.size() + 1; ++i)
      new_row[i] = row[i - 1];
    res_tab.push_back(std::move(new_row));
  }
  return res_tab;
}

simple_tab dbgen::tab_union(const simple_tab &tab1, const simple_tab &tab2) {
  simple_tab res_tab;
  res_tab.reserve(tab1.size() + tab2.size());
  res_tab.insert(res_tab.end(), tab1.cbegin(), tab1.cend());
  res_tab.insert(res_tab.end(), tab2.cbegin(), tab2.cend());
  return res_tab;
}

simple_tab dbgen::random_tab(size_t n_cols, size_t n_rows) {
  simple_tab res_tab(n_rows, std::vector<int64_t>(n_cols));
  for (size_t i = 0; i < n_rows; ++i) {
    for (size_t j = 0; j < n_cols; ++j) {
      curr_var_name += absl::Uniform<int64_t>(bitgen, 1, 5);
      res_tab[i][j] = curr_var_name;
    }
  }
  return res_tab;
}

dbgen::ts_db_list dbgen::gen_db_impl(const Formula &formula, simple_tab pos,
                                     simple_tab neg, size_t nfvs,
                                     size_t curr_ts) {
  auto visitor =
    [&, nfvs, curr_ts](auto &&arg) -> absl::flat_hash_map<size_t, database> {
    using T = std::decay_t<decltype(arg)>;
    if constexpr (any_type_equal_v<T, Formula::or_t, Formula::and_t>) {
      size_t n_rows = std::min(pos.size(), neg.size());
      const auto t1 = random_tab(nfvs, n_rows);
      const auto t2 = random_tab(nfvs, n_rows);
      absl::flat_hash_map<size_t, database> ldb, rdb;
      if constexpr (std::is_same_v<T, Formula::or_t>) {
        ldb = gen_db_impl(*arg.phil, tab_union(pos, t1), tab_union(neg, t2),
                          nfvs, curr_ts);
        rdb = gen_db_impl(*arg.phir, tab_union(t1, t2), tab_union(neg, pos),
                          nfvs, curr_ts);
      } else {
        ldb = gen_db_impl(*arg.phil, tab_union(pos, neg), tab_union(t1, t2),
                          nfvs, curr_ts);

        if (const auto *const neg_ptr = arg.phir->inner_if_neg()) {
          rdb = gen_db_impl(*neg_ptr, tab_union(neg, t1), tab_union(pos, t2),
                            nfvs, curr_ts);
        } else {
          rdb = gen_db_impl(*arg.phir, tab_union(pos, t2), tab_union(neg, t1),
                            nfvs, curr_ts);
        }
      }
      merge_ts_db_list(ldb, rdb);
      return ldb;
    } else if constexpr (std::is_same_v<T, Formula::exists_t>) {
      auto tplus = random_tab(1, pos.size());
      auto tminus = random_tab(1, neg.size());
      auto new_pos = prepend_col(pos, *tplus.begin());
      auto new_neg = prepend_col(neg, *tminus.begin());
      return gen_db_impl(*arg.phi, new_pos, new_neg, nfvs + 1, curr_ts);
    } else if constexpr (std::is_same_v<T, Formula::since_t>) {
      size_t ts_j_offset = arg.inter.random_sample(bitgen);
      if (ts_j_offset > curr_ts)
        return {};
      size_t ts_j = curr_ts - ts_j_offset;
      auto res = gen_db_impl(*arg.phir, pos, neg, nfvs, ts_j);
      const Formula *phil;
      if (const auto *ptr = arg.phil->inner_if_neg()) {
        phil = ptr;
        swap(pos, neg);
      } else {
        phil = arg.phil.get();
      }
      for (size_t ts_i = ts_j + 1; ts_i <= curr_ts; ++ts_i) {
        auto res_i = gen_db_impl(*phil, pos, neg, nfvs, ts_i);
        merge_ts_db_list(res, res_i);
      }
      return res;
    } else if constexpr (std::is_same_v<T, Formula::until_t>) {
      size_t ts_j_offset = arg.inter.random_sample(bitgen);
      size_t ts_j = curr_ts + ts_j_offset;
      auto res = gen_db_impl(*arg.phir, pos, neg, nfvs, ts_j);
      const Formula *phil;
      if (const auto *ptr = arg.phil->inner_if_neg()) {
        phil = ptr;
        swap(pos, neg);
      } else {
        phil = arg.phil.get();
      }
      for (size_t ts_i = curr_ts; ts_i < ts_j; ++ts_i) {
        auto res_i = gen_db_impl(*phil, pos, neg, nfvs, ts_i);
        merge_ts_db_list(res, res_i);
      }
      return res;
    } else if constexpr (std::is_same_v<T, Formula::pred_t>) {
      if (pos.empty())
        return {};
      database res_db;
      for (const auto &row : pos) {
        std::vector<int64_t> assignment;
        assignment.reserve(arg.pred_args.size());
        for (const auto &pred_arg : arg.pred_args) {
          if (const auto *const var = pred_arg.get_if_var()) {
            assignment.push_back(row[*var]);
          } else if (const auto *const cst = pred_arg.get_if_const()) {
            const auto *const iptr = cst->get_if_int();
            assert(iptr);
            assignment.push_back(*iptr);
          }
        }
        update_or_insert(res_db, arg.pred_name, assignment);
      }
      return {std::pair(curr_ts, res_db)};
    } else {
      throw std::runtime_error("unsupported fragment");
    }
  };
  return var2::visit(visitor, formula.val);
}


dbgen::ts_db_list dbgen::generate_database(const Formula &formula,
                                           size_t num_tups, size_t curr_ts) {
  size_t num_fvs = formula.degree();
  auto tplus = random_tab(num_fvs, num_tups);
  auto tminus = random_tab(num_fvs, num_tups);
  return gen_db_impl(formula, tplus, tminus, formula.degree(), curr_ts);
}

void dbgen::validate_formula(const Formula &formula) {
  validate_formula_impl(formula, formula.fvs());
}

int main(int argc, char *argv[]) {
  namespace fs = std::filesystem;
  absl::SetProgramUsageMessage(
    "Generate a database for a non-temporal formula (only for safe formulas)");
  absl::ParseCommandLine(argc, argv);
  fs::path fpath = absl::GetFlag(FLAGS_formula_path);
  fs::path spath = absl::GetFlag(FLAGS_sig_path);
  fs::path opath = absl::GetFlag(FLAGS_out_path);
  size_t num_tups = absl::GetFlag(FLAGS_num_satisfactions);
  signature sig = parse::signature_parser::parse(read_file(spath));
  Formula formula = Formula(read_file(fpath));
  if (fs::exists(opath) && fs::is_regular_file(opath))
    fs::remove(opath);
  auto db_gen_obj = dbgen();
  // db_gen_obj.validate_formula(formula);
  size_t num_tp = absl::GetFlag(FLAGS_num_tp);
  dbgen::ts_db_list res;
  for (size_t curr_ts = 0; curr_ts < num_tp; ++curr_ts) {
    auto new_res = db_gen_obj.generate_database(formula, num_tups, curr_ts);
    dbgen::merge_ts_db_list(res, new_res);
  }
  auto f = fmt::output_file(opath.string());
  for (size_t curr_ts = 0; curr_ts < num_tp; ++curr_ts) {
    auto &elem = res[curr_ts];
    f.print("{}\n", parse::monpoly_fmt<parse::timestamped_database>(
                      std::pair(curr_ts, std::move(elem))));
  }
}