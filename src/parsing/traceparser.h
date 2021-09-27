#ifndef TRACEPARSER_H
#define TRACEPARSER_H

#include <absl/container/flat_hash_map.h>
#include <absl/container/flat_hash_set.h>
#include <charconv>
#include <event_data.h>
#include <fmt/core.h>
#include <iterator>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace parse {

#define RULE static constexpr auto rule =
#define VALUE static constexpr auto value =

namespace {
  namespace var2 = boost::variant2;
  namespace dsl = lexy::dsl;
}// namespace

enum arg_types
{
  INT_TYPE,
  FLOAT_TYPE,
  STRING_TYPE
};
using signature = absl::flat_hash_map<std::string, std::vector<arg_types>>;
using database_tuple = std::vector<common::event_data>;
using database_elem = absl::flat_hash_set<database_tuple>;
using database = absl::flat_hash_map<std::string, database_elem>;
using timestamped_database = std::pair<size_t, database>;

class trace_parser;

class signature_parser {
  friend class trace_parser;

public:
  static signature parse(std::string_view sig);

private:
  struct int_type : lexy::token_production {
    RULE LEXY_LIT("int");
    VALUE lexy::constant(INT_TYPE);
  };

  struct float_type : lexy::token_production {
    RULE LEXY_LIT("float");
    VALUE lexy::constant(FLOAT_TYPE);
  };

  struct string_type : lexy::token_production {
    RULE LEXY_LIT("string");
    VALUE lexy::constant(STRING_TYPE);
  };

  struct sig_type : lexy::token_production, lexy::transparent_production {
    RULE dsl::p<int_type> | dsl::p<float_type> | dsl::p<string_type>;
    VALUE lexy::forward<arg_types>;
  };

  struct sig_type_list {
    using res_type = std::vector<arg_types>;
    RULE dsl::parenthesized.list(dsl::p<sig_type>, dsl::sep(dsl::comma));
    VALUE lexy::as_list<res_type>;
  };

  struct pred_name {
    using res_type = std::string;
    RULE dsl::identifier(dsl::ascii::alpha_digit_underscore);
    VALUE lexy::as_string<res_type>;
  };

  struct pred_sig {
    using res_type = std::pair<pred_name::res_type, sig_type_list::res_type>;
    RULE dsl::p<pred_name> + dsl::p<sig_type_list>;
    VALUE lexy::construct<res_type>;
  };

  struct sig_parse {
    static constexpr auto whitespace = dsl::ascii::blank / dsl::ascii::newline;
    RULE dsl::list(dsl::peek_not(dsl::eof) >> dsl::p<pred_sig>);
    static constexpr auto fold_fn = [](signature &sig,
                                       pred_sig::res_type &&psig) {
      if (!sig.insert(std::move(psig)).second)
        throw std::runtime_error("duplicate predicate");
    };
    VALUE
    lexy::fold_inplace<signature>(std::initializer_list<signature::init_type>{},
                                  fold_fn);
  };
};

class trace_parser {
public:
  explicit trace_parser(signature sig) : sig_(std::move(sig)) {}
  trace_parser() = default;
  timestamped_database parse_database(std::string_view db);

private:
  signature sig_;
  struct number_arg {
    using res_type = var2::variant<std::string_view, std::string>;
    RULE[] {
      auto exp_l = dsl::lit_c<'e'> / dsl::lit_c<'E'>;
      auto allowed_chars = dsl::digit<> / dsl::period / dsl::lit_c<'-'> /exp_l;
      return dsl::capture(dsl::while_(allowed_chars));
    }
    ();
    VALUE lexy::callback<res_type>([](auto lexeme) {
      return res_type(var2::in_place_index<0>,
                      std::string_view(lexeme.data(), lexeme.size()));
    });
  };

  struct string_arg {
    using res_type = number_arg::res_type;
    RULE dsl::quoted(dsl::ascii::alpha_digit_underscore);
    VALUE lexy::as_string<std::string> >>
      lexy::bind(lexy::construct<res_type>, var2::in_place_index<1>, lexy::_1);
  };

  struct arg_value {
    using res_type = number_arg::res_type;
    RULE dsl::peek(dsl::digit<> / dsl::lit_c<'-'>) >> dsl::p<number_arg> |
      dsl::else_ >> dsl::p<string_arg>;
    VALUE lexy::forward<res_type>;
  };

  struct arg_tuple {
    using res_type = std::vector<arg_value::res_type>;
    RULE dsl::parenthesized.list(dsl::p<arg_value>, dsl::sep(dsl::comma));
    VALUE lexy::as_list<res_type>;
  };

  struct arg_tuple_list {
    using res_type = std::vector<arg_tuple::res_type>;
    RULE dsl::list(dsl::peek_not(dsl::ascii::space / dsl::semicolon) >>
                   dsl::p<arg_tuple>);
    VALUE lexy::as_list<res_type>;
  };

  struct named_arg_tup_list {
    using res_type =
      std::pair<signature_parser::pred_name::res_type,
                absl::flat_hash_set<std::vector<common::event_data>>>;
    RULE dsl::p<signature_parser::pred_name> + dsl::p<arg_tuple_list>;
    static constexpr auto cb = lexy::callback<res_type>(
      [](std::string &&pred_name, typename arg_tuple_list::res_type &&pred_args,
         const signature &sig) {
        auto it = sig.find(pred_name);
        if (it == sig.cend()) {
          throw std::runtime_error(
            fmt::format("unknown predicate {}", pred_name));
        }
        absl::flat_hash_set<std::vector<common::event_data>> res_set;
        res_set.reserve(pred_args.size());
        const auto &tys = it->second;
        size_t n1 = tys.size();
        for (auto &tup : pred_args) {
          size_t n2 = tup.size();
          if (n1 != n2) {
            throw std::runtime_error(
              fmt::format("for predicate {} expected {} arguments, got {}",
                          pred_name, n1, n2));
          }
          std::vector<common::event_data> res_vec;
          res_vec.reserve(n1);
          for (size_t i = 0; i < n1; ++i) {
            arg_types ty = tys[i];
            switch (ty) {
              case INT_TYPE:
              case FLOAT_TYPE: {
                const std::string_view *ptr = var2::get_if<0>(&(tup[i]));
#ifndef NDEBUG
                if (!ptr)
                  throw std::runtime_error(fmt::format("expected float or int, got string"));
#endif
                if (ty == INT_TYPE) {
                  int res{};
                  const auto *fst = ptr->data(), *lst = fst + ptr->size();
                  auto [new_ptr, ec] = std::from_chars(fst, lst, res);
#ifndef NDEBUG
                  if (ec != std::errc() || new_ptr != lst) {
                    throw std::runtime_error(
                      fmt::format("expected int, got input: {}", *ptr));
                  }
#endif
                  res_vec.push_back(common::event_data::Int(res));
                } else {
                  double res{};
                  const auto *fst = ptr->data(), *lst = fst + ptr->size();
                  auto [new_ptr, ec] = std::from_chars(fst, lst, res);
#ifndef NDEBUG
                  if (ec != std::errc() || new_ptr != lst) {
                    throw std::runtime_error(
                      fmt::format("expected int, got input: {}", *ptr));
                  }
#endif
                  res_vec.push_back(common::event_data::Float(res));
                }
                break;
              }
              case STRING_TYPE: {
                auto *ptr = var2::get_if<1>(&(tup[i]));
#ifndef NDEBUG
                if (!ptr)
                  throw std::runtime_error("expected string, got float or int");
#endif
                res_vec.push_back(common::event_data::String(std::move(*ptr)));
                break;
              }
            }
          }
          res_set.insert(std::move(res_vec));
        }
        return std::make_pair(std::move(pred_name), std::move(res_set));
      });
    VALUE lexy::bind(cb, lexy::_1, lexy::_2, lexy::parse_state);
  };


  struct db_parse {
    RULE dsl::list(dsl::p<named_arg_tup_list>, dsl::sep(dsl::ascii::blank));
    static constexpr auto fold_fn = [](database &db,
                                       named_arg_tup_list::res_type &&elem) {
      auto it = db.find(elem.first);
      if (it == db.end())
        db.insert(std::move(elem));
      else
        it->second.insert(std::make_move_iterator(elem.second.begin()),
                          std::make_move_iterator(elem.second.end()));
    };
    VALUE
    lexy::fold_inplace<database>(std::initializer_list<database::init_type>{},
                                 fold_fn);
  };

  struct ts_parse {
    RULE dsl::integer<size_t>(dsl::digits<>);
    VALUE lexy::as_integer<size_t>;
  };

  struct ts_db_parse {
    RULE dsl::at_sign + dsl::p<ts_parse> + dsl::ascii::blank +
      dsl::p<db_parse> + dsl::semicolon +
      dsl::if_(dsl::ascii::newline) + dsl::eof;
    VALUE lexy::construct<timestamped_database>;
  };
#undef RULE
#undef VALUE
};
}// namespace parse


#endif