#include "percemon/parser.hpp"

#include "utils/visit.hpp"

#include "actions.hpp"
#include "error_messages.hpp"
#include "grammar.hpp"

#include <tao/pegtl.hpp>                    // for pegtl
#include <tao/pegtl/contrib/parse_tree.hpp> // for basic_node<>::children_t

#include <algorithm> // for max
#include <iostream>
#include <utility> // for move

/// Private namespace for the parser core.
namespace {

namespace grammar = percemon::grammar;
namespace parser  = percemon::parser;
namespace actions = percemon::parser::actions;
namespace peg     = tao::pegtl;

/// This function takes in some PEGTL compliant input and outputs a concrete context.
///
/// To do this, it first generates a parse tree using PEGTL's in-built parse_tree
/// generator. Then, it transforms this parse_tree into the concrete context with the
/// abstract syntax tree.
template <typename ParseInput>
std::unique_ptr<percemon::Context> _parse(ParseInput&& input) {
  auto global_state     = actions::GlobalContext{};
  auto top_local_state  = actions::Context{};
  top_local_state.level = 0;

  try {
    peg::parse<grammar::Specification, actions::action>(
        input, global_state, top_local_state);
  } catch (const peg::parse_error& e) {
    const auto p = e.positions().front();

    std::cerr << e.what() << std::endl
              << input.line_at(p) << '\n'
              << std::setw(p.column) << '^' << std::endl;
    return {};
  }

  auto ctx = std::make_unique<percemon::Context>();

  ctx->defined_formulas = std::move(global_state.defined_formulas);
  ctx->monitors         = std::move(global_state.monitors);
  ctx->settings         = std::move(global_state.settings);

  return ctx;
}

} // namespace

namespace percemon {
std::unique_ptr<Context> Context::from_string(std::string_view input) {
  tao::pegtl::string_input in(input, "from_content");
  return _parse(in);
}

std::unique_ptr<Context> Context::from_file(const fs::path& input) {
  tao::pegtl::file_input in(input);
  return _parse(in);
}

} // namespace percemon
