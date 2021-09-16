#ifndef MFORMULA_H
#define MFORMULA_H

#include <absl/container/flat_hash_set.h>
#include <boost/hana/functional/overload.hpp>
#include <boost/hana/functional/overload_linearly.hpp>
#include <boost/variant2/variant.hpp>
#include <formula.h>
#include <stdexcept>
#include <table.h>
#include <type_traits>
#include <util.h>
#include <variant>
#include <vector>

namespace mfo {
namespace detail {
  namespace hana = boost::hana;

  namespace var2 = boost::variant2;
  using std::vector;
  using var2::variant;

  using fo::event_data;
  using fo::Formula;
  using fo::fv_set;
  using fo::Interval;
  using fo::name;
  using fo::Term;
  using fo::detail::ptr_type;

  using table_type = table<event_data>;

  // TODO: implement this later together with meval
  struct MBuf2 {};

  // TODO: implement this later together with meval
  struct MSaux {};

  // TODO: implement this later together with meval
  struct MUaux {};

  class MState;

  struct MRel {
    table_type tab;
  };

  struct MPred {
    name pred_name;
    vector<Term> pred_args;
  };

  struct MAnd {
    bool is_positive;
    MBuf2 buf;
    ptr_type<MState> l_state, r_state;
  };

  // TODO: implement this later together with meval
  struct MAndAssign {};

  // TODO: implement this later together with meval
  struct MAndRel {};


  struct MOr {
    ptr_type<MState> l_state, r_state;
    MBuf2 buf;
  };

  struct MNeg {
    ptr_type<MState> state;
  };

  struct MExists {
    size_t bound_var;
    ptr_type<MState> state;
  };

  struct MPrev {
    Interval inter;
    bool is_first;
    vector<table_type> past_data;
    vector<size_t> past_ts;
    ptr_type<MState> state;
  };

  struct MNext {
    Interval inter;
    bool is_first;
    vector<size_t> past_ts;
    ptr_type<MState> state;
  };

  struct MSince {};

  struct MUntil {};


  class MState {
  public:
    explicit MState(const Formula &formula) { state = fo_2_state(formula, 0); }

  private:
    using val_type = variant<var2::monostate, MRel, MPred, MOr, MExists, MPrev,
                             MNext, MNeg, MAndRel, MAndAssign, MAnd>;

    explicit MState(val_type &&state) : state(std::move(state)){};

    static inline constexpr auto uniq = [](auto &&arg) -> decltype(auto) {
      return std::make_unique<MState>(std::forward<decltype(arg)>(arg));
    };

    static val_type convert_eq_to_state(const fo::Eq &arg) {
      const auto &t1 = arg.l, &t2 = arg.r;
      if (t1.is_const() && t2.is_const()) {
        if (t1.get_const() == t2.get_const())
          return MRel{table_type::unit_table()};
        else
          return MRel{table_type::empty_table()};
      } else if (t1.is_var() && t2.is_const())
        return MRel{table_type::singleton_table(t1.get_var(), t2.get_const())};
      else if (t1.is_const() && t2.is_var())
        return MRel{table_type::singleton_table(t2.get_var(), t1.get_const())};
      else
        throw std::runtime_error("safe assignment violated");
    }

    static val_type convert_and_to_state(const fo::And &arg,
                                         size_t n_bound_vars) {
      const auto &phil = arg.phil, &phir = arg.phir;
      if (phir->is_safe_assignment(phil->fvs())) {
        return MAndRel{};
      } else if (phir->is_safe_formula()) {
        auto l_state = MState(fo_2_state(*phil, n_bound_vars)),
             r_state = MState(fo_2_state(*phir, n_bound_vars));
        return MAnd{true, MBuf2{}, uniq(std::move(l_state)),
                    uniq(std::move(r_state))};
      } else if (phir->is_constraint()) {
        return MAndRel{};
      } else if (const auto *phir_neg = var2::get_if<fo::Neg>(&phir->val)) {
        auto l_state = MState(fo_2_state(*phil, n_bound_vars)),
             r_state = MState(fo_2_state(*phir_neg->phi, n_bound_vars));
        return MAnd{false, MBuf2{}, uniq(std::move(l_state)),
                    uniq(std::move(r_state))};
      } else {
        throw std::runtime_error("trying to initialize state with invalid and");
      }
    }

    static val_type fo_2_state(const Formula &formula, size_t n_bound_vars) {
      auto visitor1 = [n_bound_vars](auto &&arg) -> val_type {
        using T = std::decay_t<decltype(arg)>;
        using std::is_same_v;

        if constexpr (is_same_v<T, fo::Neg>) {
          if (arg.phi->fvs().empty())
            return MNeg{uniq(MState{fo_2_state(*arg.phi, n_bound_vars)})};
          else
            return MRel{table_type::empty_table()};
        } else if constexpr (is_same_v<T, fo::Eq>) {
          return convert_eq_to_state(arg);
        } else if constexpr (is_same_v<T, fo::Pred>) {
          return MPred{arg.pred_name, arg.pred_args};
        } else if constexpr (is_same_v<T, fo::Or>) {
          return MOr{uniq(MState(*arg.phil)), uniq(MState(*arg.phir)), MBuf2()};
        } else if constexpr (is_same_v<T, fo::And>) {
          return convert_and_to_state(arg, n_bound_vars);
        } else if constexpr (is_same_v<T, fo::Exists>) {
          auto new_state = uniq(MState(fo_2_state(*arg.phi, n_bound_vars + 1)));
          return MExists{n_bound_vars, std::move(new_state)};
        } else if constexpr (is_same_v<T, fo::Prev>) {
          auto new_state = uniq(MState(fo_2_state(*arg.phi, n_bound_vars)));
          return MPrev{arg.inter, true, {}, {}, std::move(new_state)};
        } else if constexpr (is_same_v<T, fo::Next>) {
          auto new_state = uniq(MState{fo_2_state(*arg.phi, n_bound_vars)});
          return MNext{arg.inter, true, {}, std::move(new_state)};
        } else if constexpr (is_same_v<T, fo::Since>) {
          return MPred{};
        } else if constexpr (is_same_v<T, fo::Until>) {
          return MPred{};
        } else {
          throw std::runtime_error("not safe");
        }
      };
      return var2::visit(visitor1, formula.val);
    }
    val_type state;
  };

}// namespace detail

using detail::MAnd;
using detail::MAndAssign;
using detail::MAndRel;
using detail::MExists;
using detail::MNeg;
using detail::MNext;
using detail::MOr;
using detail::MPred;
using detail::MPrev;
using detail::MRel;
using detail::MSince;
using detail::MState;
using detail::MUntil;
using detail::table_type;

}// namespace mfo

#endif