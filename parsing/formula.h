#ifndef FORMULA_H
#define FORMULA_H

#include <algorithm>
#include <boost/hana.hpp>
#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace fo {
namespace hana = boost::hana;
using std::string;
using std::literals::string_view_literals::operator""sv;

#define SINGLE_ARG(...) __VA_ARGS__

#define LEAF_NODE(NAME)                                                        \
  struct NAME {                                                                \
    static constexpr std::string_view get_type_name() {                        \
      return "" #NAME ""sv;                                                    \
    };                                                                         \
    BOOST_HANA_DEFINE_STRUCT(NAME);                                            \
    NAME() = default;                                                          \
  };

#define MAKE_NODE(NAME, TYPE)                                                  \
  struct NAME {                                                                \
    static constexpr std::string_view get_type_name() {                        \
      return "" #NAME ""sv;                                                    \
    };                                                                         \
    BOOST_HANA_DEFINE_STRUCT(NAME, (TYPE, val));                               \
    NAME(TYPE &&val) : val(std::move(val)){};                                  \
    NAME() = default;                                                          \
  };

string quote(std::string s);
string to_string(const std::string &s);
template <typename T> string join(T &&xs, string sep);
template <typename T>
std::enable_if_t<hana::is_a<hana::tuple_tag, T>, string> to_string(const T &x);
template <typename T>
std::enable_if_t<hana::Struct<T>::value, string> to_string(const T &x);
template <typename... T> string to_string(const std::variant<T...> &v);
template <typename T> string to_string(const std::unique_ptr<T> &p);
template <typename T> string to_string(const std::vector<T> &v);

template <typename T>
auto to_string(const T &v) -> decltype(std::to_string(v)) {
  return std::to_string(v);
}

template <typename T> string join(T &&xs, string sep) {
  return hana::fold(hana::intersperse(std::forward<T>(xs), sep), "",
                    hana::_ + hana::_);
}

template <typename T>
std::enable_if_t<hana::is_a<hana::tuple_tag, T>, string> to_string(const T &x) {
  auto tup_strs =
      hana::transform(x, [](const auto &x) { return to_string(x); });
  return join(std::move(tup_strs), " ");
}

template <typename T>
std::enable_if_t<hana::Struct<T>::value, string> to_string(const T &x) {
  auto mem_strs = hana::transform(hana::keys(x), [&](auto name) {
    auto const &member = hana::at_key(x, name);
    return to_string(member);
  });
  return string(T::get_type_name()) + " (" + join(std::move(mem_strs), " ") +
         ")";
}

template <typename... T> string to_string(const std::variant<T...> &v) {
  return std::visit([](const auto &x) { return to_string(x); }, v);
}

template <typename T> string to_string(const std::unique_ptr<T> &p) {
  if (!p) {
    return "nil";
  }
  return to_string(*p);
}

template <typename T> string to_string(const std::vector<T> &v) {
  if (v.size() == 0)
    return "";
  else {
    string acc;
    auto it = v.begin();
    acc += to_string(*it);
    for (; it < v.end(); ++it) {
      acc += to_string(*it);
    }
    return acc;
  }
}

template <typename T> using ptr_type = std::unique_ptr<T>;

enum agg_op { Cnt, Min, Max, Sum, Avg, Med };

string to_string(const agg_op &o);

MAKE_NODE(OBnd, size_t);
MAKE_NODE(CBnd, size_t);
LEAF_NODE(Inf);

using bound = std::variant<OBnd, CBnd, Inf>;
using interval = hana::tuple<bound, bound>;

enum tcst { TInt, TStr, TFloat };
enum tcl { TNum, TAny };

string to_string(const tcst &o);
string to_string(const tcl &o);

MAKE_NODE(TSymb, SINGLE_ARG(hana::tuple<tcl, int>));
MAKE_NODE(TCst, tcst);
using tsymb = std::variant<TSymb, TCst>;

struct term;
using rec_term = ptr_type<term>;
using rec_term_pair = hana::tuple<rec_term, rec_term>;
using rec_term_vec = std::vector<ptr_type<term>>;

MAKE_NODE(Var, string);
MAKE_NODE(CInt, int);
MAKE_NODE(CStr, string);
MAKE_NODE(CFloat, double);
using cst_val = std::variant<CInt, CStr, CFloat>;
MAKE_NODE(Cst, cst_val);

MAKE_NODE(F2i, rec_term);
MAKE_NODE(I2f, rec_term);
MAKE_NODE(UMinus, rec_term);

MAKE_NODE(Plus, rec_term_pair);
MAKE_NODE(Minus, rec_term_pair);
MAKE_NODE(Mult, rec_term_pair);
MAKE_NODE(Div, rec_term_pair);
MAKE_NODE(Mod, rec_term_pair);
using term_val =
    std::variant<Var, Cst, F2i, I2f, Plus, Minus, UMinus, Mult, Div, Mod>;
MAKE_NODE(term, term_val);
MAKE_NODE(predicate, SINGLE_ARG(hana::tuple<string, size_t, rec_term_vec>));

struct formula;
struct regex;
using rec_regex = ptr_type<regex>;
using rec_regex_pair = hana::tuple<rec_regex, rec_regex>;
using rec_formula = ptr_type<formula>;
using rec_formula = ptr_type<formula>;
using rec_formula_pair = hana::tuple<rec_formula, rec_formula>;
using rec_inter_formula = hana::tuple<interval, rec_formula>;

MAKE_NODE(Equal, rec_term_pair);
MAKE_NODE(Less, rec_term_pair);
MAKE_NODE(LessEq, rec_term_pair);
MAKE_NODE(Pred, predicate);
MAKE_NODE(Let, SINGLE_ARG(hana::tuple<predicate, rec_formula, rec_formula>));
MAKE_NODE(Neg, rec_formula);
MAKE_NODE(And, rec_formula_pair);
MAKE_NODE(Or, rec_formula_pair);
MAKE_NODE(Implies, rec_formula_pair);
MAKE_NODE(Equiv, rec_formula_pair);
MAKE_NODE(Exists, SINGLE_ARG(hana::tuple<std::vector<string>, rec_formula>));
MAKE_NODE(ForAll, SINGLE_ARG(hana::tuple<std::vector<string>, rec_formula>));
MAKE_NODE(Aggreg, SINGLE_ARG(hana::tuple<tsymb, string, agg_op, string,
                                         std::vector<string>, rec_formula>));
MAKE_NODE(Prev, rec_inter_formula);
MAKE_NODE(Next, rec_inter_formula);
MAKE_NODE(Eventually, rec_inter_formula);
MAKE_NODE(Once, rec_inter_formula);
MAKE_NODE(Always, rec_inter_formula);
MAKE_NODE(PastAlways, rec_inter_formula);
MAKE_NODE(Since, SINGLE_ARG(hana::tuple<interval, rec_formula, rec_formula>));
MAKE_NODE(Until, SINGLE_ARG(hana::tuple<interval, rec_formula, rec_formula>));
MAKE_NODE(Frex, SINGLE_ARG(hana::tuple<interval, rec_regex>));
MAKE_NODE(Prex, SINGLE_ARG(hana::tuple<interval, rec_regex>));
using formula_val =
    std::variant<Equal, Less, LessEq, Pred, Let, Neg, And, Or, Implies, Equiv,
                 Exists, ForAll, Aggreg, Prev, Next, Eventually, Once, Always,
                 PastAlways, Since, Until, Frex, Prex>;
MAKE_NODE(formula, formula_val);

LEAF_NODE(Wild);
MAKE_NODE(RegTest, rec_formula);
MAKE_NODE(RegConcat, rec_regex_pair);
MAKE_NODE(RegPlus, rec_regex_pair);
MAKE_NODE(RegStar, rec_regex);
using regex_val = std::variant<Wild, RegTest, RegConcat, RegPlus, RegStar>;
MAKE_NODE(regex, regex_val);
} // namespace fo
#endif