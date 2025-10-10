#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <fmt/format.h>

#include "percemon/ast.hpp"
#include "percemon/fmt.hpp"

using namespace percemon::ast;

TEST_CASE("AST nodes throw exceptions when constructed badly", "[ast][except]") {
  SECTION("TimeBounds are constructed with proper relational operators") {
    REQUIRE_NOTHROW(Var_x{"x_1"} - C_TIME);

    REQUIRE_THROWS(TimeBound{Var_x{"x_1"} - C_TIME, ComparisonOp::EQ, 1.0});
    REQUIRE_THROWS(TimeBound{Var_x{"x_1"} - C_TIME, ComparisonOp::NE, 1.0});

    REQUIRE_NOTHROW(greater_than(Var_x{"x_1"} - C_TIME, 1.0));
    REQUIRE_NOTHROW(greater_than_eq(Var_x{"x_1"} - C_TIME, 1.0));
    REQUIRE_NOTHROW(less_than(Var_x{"x_1"} - C_TIME, 1.0));
    REQUIRE_NOTHROW(less_than_eq(Var_x{"x_1"} - C_TIME, 1.0));
  }

  SECTION("FrameBounds are constructed with proper relational operators") {
    REQUIRE_NOTHROW(Var_f{"f_1"} - C_FRAME);

    REQUIRE_NOTHROW(greater_than(Var_f{"f_1"} - C_FRAME, 4));
    REQUIRE_NOTHROW(greater_than_eq(Var_f{"f_1"} - C_FRAME, 4));
    REQUIRE_NOTHROW(less_than(Var_f{"f_1"} - C_FRAME, 4));
    REQUIRE_NOTHROW(less_than_eq(Var_f{"f_1"} - C_FRAME, 4));

    REQUIRE_THROWS(FrameBound{Var_f{"f_1"} - C_FRAME, ComparisonOp::EQ, 4});
    REQUIRE_THROWS(FrameBound{Var_f{"f_1"} - C_FRAME, ComparisonOp::NE, 4});
  }

  SECTION("CompareId are constructed with proper relational operators") {
    REQUIRE_NOTHROW(Var_id{"id_1"} == Var_id{"id_2"});
    REQUIRE_NOTHROW(Var_id{"id_1"} != Var_id{"id_2"});
    REQUIRE_THROWS(CompareId{Var_id{"id_1"}, ComparisonOp::GT, Var_id{"id_2"}});
    REQUIRE_THROWS(CompareId{Var_id{"id_1"}, ComparisonOp::GE, Var_id{"id_2"}});
    REQUIRE_THROWS(CompareId{Var_id{"id_1"}, ComparisonOp::LT, Var_id{"id_2"}});
    REQUIRE_THROWS(CompareId{Var_id{"id_1"}, ComparisonOp::LE, Var_id{"id_2"}});
  }

  SECTION("CompareClass is constructed with proper relational operators") {
    const auto id1 = Var_id{"id_1"};
    const auto id2 = Var_id{"id_2"};

    enum class Classification : std::uint8_t { PED, CAR };

    REQUIRE_NOTHROW(equal(Class{id1}, Class{id2}));
    REQUIRE_NOTHROW(not_equal(Class{id1}, Class{id2}));
    REQUIRE_NOTHROW(equal(Class{id1}, static_cast<int>(Classification::PED)));
    REQUIRE_NOTHROW(not_equal(Class{id2}, static_cast<int>(Classification::CAR)));
  }

  SECTION("Prob(id) can be constructed with or without a scalar multiplier") {
    const auto id = Var_id{"id_1"};
    REQUIRE_NOTHROW(Prob(id));
    // REQUIRE_NOTHROW(Prob(id) * 20.0);
    // SECTION("Multiplying Prob(id) repeatedly stacks up") {
    //   {
    //     auto prob = Prob(id);
    //     REQUIRE(prob.scale == Catch::Approx(1.0));
    //     prob *= 2.0;
    //     REQUIRE(prob.scale == Catch::Approx(2.0));
    //     prob *= 3.0;
    //     REQUIRE(prob.scale == Catch::Approx(6.0));
    //   }
    //   {
    //     Prob prob = Prob(id) * 20.0;
    //     REQUIRE(prob.scale == Catch::Approx(20.0));
    //     prob *= 2.0;
    //     REQUIRE(prob.scale == Catch::Approx(40.0));
    //     prob *= 3.0;
    //     REQUIRE(prob.scale == Catch::Approx(120.0));
    //   }
    // }
  }

  SECTION("CompareProb is constructed with proper relational operators") {
    const auto id1 = Prob{Var_id{"id_1"}};
    const auto id2 = Prob{Var_id{"id_2"}};

    REQUIRE_NOTHROW(greater_than(id1, 0.2));
    REQUIRE_NOTHROW(greater_than_eq(id1, 0.3));
    REQUIRE_NOTHROW(less_than(id2, 0.5));
    REQUIRE_NOTHROW(less_than_eq(id2, 1.0));
    REQUIRE_NOTHROW(greater_than(id1, id2));
    REQUIRE_NOTHROW(greater_than_eq(id1, id2));
    REQUIRE_NOTHROW(less_than(id1, id2));
    REQUIRE_NOTHROW(less_than_eq(id1, id2));
  }

  SECTION("Pins have either Var_x or Var_f or both") {
    REQUIRE_NOTHROW(Pin{Var_f{"f1"}, CONST_TRUE});
    REQUIRE_NOTHROW(Pin{Var_x{"x1"}, CONST_TRUE});
    REQUIRE_NOTHROW(Pin{Var_x{"x"}, Var_f{"f1"}, CONST_TRUE});
    REQUIRE_NOTHROW(Pin{{}, Var_f{"f1"}, CONST_TRUE});
    REQUIRE_NOTHROW(Pin{Var_x{"x"}, {}, CONST_TRUE});

    REQUIRE_THROWS_AS(Pin({}, {}, CONST_TRUE), std::invalid_argument);
  }
}

TEST_CASE("AST nodes are printed correctly", "[ast][fmt]") {
  SECTION("Primitives in the syntax") {
    REQUIRE("true" == (Const{true}).to_string());
    REQUIRE("false" == (Const{false}).to_string());
    REQUIRE("C_TIME" == (C_TIME).to_string());
    REQUIRE("C_FRAME" == (C_FRAME).to_string());
    REQUIRE("f_1" == (Var_f{"f_1"}).to_string());
    REQUIRE("x_1" == (Var_x{"x_1"}).to_string());
    REQUIRE("id_1" == (Var_id{"id_1"}).to_string());
  }

  const auto x1  = Var_x{"x_1"};
  const auto f1  = Var_f{"f_1"};
  const auto id1 = Var_id{"id_1"};
  const auto x2  = Var_x{"x_2"};
  const auto f2  = Var_f{"f_2"};
  const auto id2 = Var_id{"id_2"};

  SECTION("Functions on Var_id") {
    REQUIRE("Prob(id_1)" == Prob{id1}.to_string());
    // REQUIRE(fmt::format("{} * Prob(id_1)", 4.0) == fmt::to_string(4.0 * Prob(id1)));

    REQUIRE("Class(id_2)" == (Class{id2}.to_string()));
  }

  SECTION("TimeBounds, FrameBounds, and other Comparisons") {
    REQUIRE(
        fmt::format("(x_1 - C_TIME >= {})", 1.0) ==
        (greater_than_eq(x1 - C_TIME, 1.0).to_string()));
    REQUIRE(
        fmt::format("(x_2 - C_TIME > {})", 1.0) ==
        (greater_than(x2 - C_TIME, 1.0).to_string()));
    REQUIRE(
        fmt::format("(x_1 - C_TIME < {})", 1.0) ==
        (less_than_eq(x1 - C_TIME, 1.0).to_string()));
    REQUIRE(
        fmt::format("(x_2 - C_TIME < {})", 1.0) ==
        (less_than(x2 - C_TIME, 1.0).to_string()));

    REQUIRE(("(f_1 - C_FRAME >= 5)" == (greater_than_eq(f1 - C_FRAME, 5).to_string())));
    REQUIRE("(f_2 - C_FRAME,5)" == (greater_than(f2 - C_FRAME, 5).to_string()));
    REQUIRE("(f_1 - C_FRAME,5)" == (less_than_eq(f1 - C_FRAME, 5).to_string()));
    REQUIRE("(f_2 - C_FRAME,5)" == (less_than(f2 - C_FRAME, 5).to_string()));

    REQUIRE("(id_1 == id_1)" == (equal(id1, id1).to_string()));
    REQUIRE("(id_1 != id_2)" == (not_equal(id1, id2).to_string()));
    REQUIRE(
        "(Class(id_1) == Class(id_2))" == (equal(Class(id1), Class(id2)).to_string()));
    REQUIRE(
        "(Class(id_1) != Class(id_2))" ==
        (not_equal(Class(id1), Class(id2)).to_string()));
    REQUIRE(
        "(Prob(id_1) >= Prob(id_2))" ==
        (greater_than_eq(Prob(id1), Prob(id2)).to_string()));
    REQUIRE(
        "(Prob(id_1) > Prob(id_2))" ==
        (greater_than(Prob(id1), Prob(id2)).to_string()));
    REQUIRE(
        "(Prob(id_1) <= Prob(id_2))" ==
        (less_than_eq(Prob(id1), Prob(id2)).to_string()));
    REQUIRE(
        "(Prob(id_1) < Prob(id_2))" == (less_than(Prob(id1), Prob(id2)).to_string()));
  }

  SECTION("Objects and quantifiers over them") {
    REQUIRE(
        "EXISTS {id_1, id_2, id_3}" ==
        (Exists{std::vector<Var_id>{{"id_1"}, {"id_2"}, {"id_3"}}, CONST_TRUE})
            .to_string());
    REQUIRE(
        "FORALL {id_1, id_2, id_3}" ==
        (Forall{std::vector<Var_id>{{"id_1"}, {"id_2"}, {"id_3"}}, CONST_TRUE})
            .to_string());
  }

  SECTION("Pinned variables") {
    REQUIRE(
        "{x_1, f_1} . false" ==
        (Pin{Var_x{"x_1"}, Var_f{"f_1"}, CONST_FALSE}).to_string());
    REQUIRE("{_, f_1} . false" == (Pin{Var_f{"f_1"}, CONST_FALSE}).to_string());
    REQUIRE("{x_1, _} . false" == (Pin{Var_x{"x_1"}, CONST_FALSE}).to_string());
    REQUIRE(
        fmt::format("{{x_1, _}} . (x_1 - C_TIME > {})", 10.0) ==
        ((
             Pin{Var_x{"x_1"},
                 std::make_shared<TimeBound>(greater_than(x1 - C_TIME, 10.0))}))
            .to_string());
  }

  SECTION("Quantifiers over objects pinned at frames") {
    REQUIRE(
        "EXISTS {id_1, id_2} @ {x_1, f_1} . false" ==
        (Exists{
             std::vector<Var_id>{{"id_1"}, {"id_2"}},
             std::make_shared<Pin>(Var_x{"x_1"}, Var_f{"f_1"}, CONST_FALSE)})
            .to_string());
  }

  SECTION("Time bounds over frames and time") {
    REQUIRE(
        fmt::format("(x_1 - C_TIME >= {})", 2.0) ==
        (greater_than_eq(Var_x{"x_1"} - C_TIME, 2.0)).to_string());

    {
      auto [x, f] = std::make_tuple(Var_x{"x_1"}, Var_f{"f_1"});
      auto phi =
          Pin{x, f, std::make_shared<TimeBound>(greater_than_eq(x - C_TIME, 2.0))};
      REQUIRE(
          fmt::format("{{x_1, f_1}} . (x_1 - C_TIME >= {})", 2.0) == phi.to_string());
    }
  }
}
