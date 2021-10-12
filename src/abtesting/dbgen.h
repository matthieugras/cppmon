#ifndef CPPMON_DBGEN_H
#define CPPMON_DBGEN_H

#include <formula.h>
#include <traceparser.h>

class dbgen {
public:
  using simple_tab = std::vector<std::vector<int>>;
  using ts_db_list = absl::flat_hash_map<size_t, parse::database>;
  static void merge_ts_db_list(ts_db_list &ts_dbs1, ts_db_list &ts_dbs2);
  ts_db_list generate_database(const fo::Formula &formula, size_t num_tups,
                               size_t curr_ts);
  void validate_formula(const fo::Formula &formula);

private:
  absl::InsecureBitGen bitgen;
  int curr_var_name{0};

  struct sub_form_info {
    absl::flat_hash_set<fo::name> pred_names;
  };

  sub_form_info validate_formula_impl(const fo::Formula &formula,
                                      const fo::fv_set &all_fvs);

  void update_or_insert(parse::database &db, const std::string &pred_name,
                        const std::vector<int> &pred_args);

  void destructive_merge(parse::database &db1, parse::database &&db2);

  simple_tab prepend_col(const simple_tab &tab,
                         const std::vector<int> &col_vals);

  simple_tab tab_union(const simple_tab &tab1, const simple_tab &tab2);

  simple_tab random_tab(size_t n_cols, size_t n_rows);

  ts_db_list gen_db_impl(const fo::Formula &formula, simple_tab pos,
                         simple_tab neg, size_t nfvs, size_t curr_ts);
};

#endif
