#include <catch2/catch.hpp>

#include "percemon/ast.hpp"
#include "percemon/fmt.hpp"

using namespace percemon::ast;

TEST_CASE("AST nodes throw exceptions when constructed badly", "[ast][except]") {
  SECTION("TimeBounds are constructed with proper relational operators") {
    REQUIRE_NOTHROW(Var_x{"1"} - C_TIME{});
    REQUIRE_NOTHROW(Var_x{"1"} - C_TIME{} > 1.0);
    REQUIRE_NOTHROW(Var_x{"1"} - C_TIME{} >= 1.0);
    REQUIRE_NOTHROW(1.0 <= Var_x{"1"} - C_TIME{});
    REQUIRE_NOTHROW(1.0 < Var_x{"1"} - C_TIME{});
    REQUIRE_THROWS(TimeBound{Var_x{"1"}, ComparisonOp::EQ, 1.0});
    REQUIRE_THROWS(TimeBound{Var_x{"1"}, ComparisonOp::NE, 1.0});
    REQUIRE_THROWS(Var_x{"1"} - C_TIME{} > -1.0);
  }

  SECTION("FrameBounds are constructed with proper relational operators") {
    REQUIRE_NOTHROW(Var_f{"1"} - C_FRAME{});
    REQUIRE_NOTHROW(Var_f{"1"} - C_FRAME{} > 4);
    REQUIRE_NOTHROW(Var_f{"1"} - C_FRAME{} >= 4);
    REQUIRE_NOTHROW(4 < Var_f{"1"} - C_FRAME{});
    REQUIRE_NOTHROW(4 <= Var_f{"1"} - C_FRAME{});
    REQUIRE_THROWS(FrameBound{Var_f{"1"}, ComparisonOp::EQ, 4});
    REQUIRE_THROWS(FrameBound{Var_f{"1"}, ComparisonOp::NE, 4});
  }

  SECTION("CompareId are constructed with proper relational operators") {
    REQUIRE_NOTHROW(Var_id{"1"} == Var_id{"2"});
    REQUIRE_NOTHROW(Var_id{"1"} != Var_id{"2"});
    REQUIRE_THROWS(CompareId{Var_id{"1"}, ComparisonOp::GT, Var_id{"2"}});
    REQUIRE_THROWS(CompareId{Var_id{"1"}, ComparisonOp::GE, Var_id{"2"}});
    REQUIRE_THROWS(CompareId{Var_id{"1"}, ComparisonOp::LT, Var_id{"2"}});
    REQUIRE_THROWS(CompareId{Var_id{"1"}, ComparisonOp::LE, Var_id{"2"}});
  }

  SECTION("CompareClass is constructed with proper relational operators") {
    const auto id1 = Var_id{"1"};
    const auto id2 = Var_id{"2"};

    enum class Classification : int { PED, CAR };

    REQUIRE_NOTHROW(Class(id1) == Class(id2));
    REQUIRE_NOTHROW(Class(id1) != Class(id2));
    REQUIRE_NOTHROW(Class(id1) == static_cast<int>(Classification::PED));
    REQUIRE_NOTHROW(Class(id2) != static_cast<int>(Classification::CAR));
    REQUIRE_NOTHROW(static_cast<int>(Classification::PED) == Class(id1));
    REQUIRE_NOTHROW(static_cast<int>(Classification::CAR) != Class(id2));
  }

  SECTION("Prob(id) can be constructed with or without a scalar multiplier") {
    const auto id = Var_id{"1"};
    REQUIRE_NOTHROW(Prob(id));
    REQUIRE_NOTHROW(Prob(id) * 20.0);
    SECTION("Multiplying Prob(id) repeatedly stacks up") {
      {
        auto prob = Prob(id);
        REQUIRE(prob.scale == Approx(1.0));
        prob *= 2.0;
        REQUIRE(prob.scale == Approx(2.0));
        prob *= 3.0;
        REQUIRE(prob.scale == Approx(6.0));
      }
      {
        auto prob = Prob(id) * 20.0;
        REQUIRE(prob.scale == Approx(20.0));
        prob *= 2.0;
        REQUIRE(prob.scale == Approx(40.0));
        prob *= 3.0;
        REQUIRE(prob.scale == Approx(120.0));
      }
    }
  }

  SECTION("CompareProb is constructed with proper relational operators") {
    const auto id1 = Prob(Var_id{"1"});
    const auto id2 = Prob(Var_id{"2"});

    REQUIRE_NOTHROW(id1 > 0.2);
    REQUIRE_NOTHROW(id1 >= 0.3);
    REQUIRE_NOTHROW(id2 < 0.5);
    REQUIRE_NOTHROW(id2 <= 1.0);
    REQUIRE_NOTHROW(id1 > id2);
    REQUIRE_NOTHROW(id1 >= id2);
    REQUIRE_NOTHROW(id1 < id2);
    REQUIRE_NOTHROW(id1 <= id2);
  }

  SECTION("Pins have either Var_x or Var_f or both") {
    REQUIRE_NOTHROW(Pin{Var_f{"f1"}});
    REQUIRE_NOTHROW(Pin{Var_x{"x1"}});
    REQUIRE_NOTHROW(Pin{Var_x{"x"}, Var_f{"f1"}});
    REQUIRE_NOTHROW(Pin{{}, Var_f{"f1"}});
    REQUIRE_NOTHROW(Pin{Var_x{"x"}, {}});

    REQUIRE_THROWS_AS(Pin({}, {}), std::invalid_argument);
  }

  SECTION("Cannot have an expression attached to the Pin and the Quantifier too") {
    auto phi = Var_x{"1"} - C_TIME{} >= 1.0;
    auto pin = Pin{Var_x{"1"}}.dot(phi);
    REQUIRE_NOTHROW(Exists{{"1"}}.at({Var_x{"1"}}).dot(phi));
    REQUIRE_THROWS_AS(Forall{{"1"}}.at(pin).dot(phi), std::invalid_argument);
  }
}

TEST_CASE("AST nodes are printed correctly", "[ast][fmt]") {
  SECTION("Primitives in the syntax") {
    REQUIRE("true" == fmt::to_string(Const{true}));
    REQUIRE("false" == fmt::to_string(Const{false}));
    REQUIRE("C_TIME" == fmt::to_string(C_TIME{}));
    REQUIRE("C_FRAME" == fmt::to_string(C_FRAME{}));
    REQUIRE("f_1" == fmt::to_string(Var_f{"1"}));
    REQUIRE("x_1" == fmt::to_string(Var_x{"1"}));
    REQUIRE("id_1" == fmt::to_string(Var_id{"1"}));
  }

  const auto x1  = Var_x{"1"};
  const auto f1  = Var_f{"1"};
  const auto id1 = Var_id{"1"};
  const auto x2  = Var_x{"2"};
  const auto f2  = Var_f{"2"};
  const auto id2 = Var_id{"2"};

  SECTION("Functions on Var_id") {
    REQUIRE("Prob(id_1)" == fmt::to_string(Prob(id1)));
    REQUIRE("4.0 * Prob(id_1)" == fmt::to_string(4.0 * Prob(id1)));

    REQUIRE("Class(id_2)" == fmt::to_string(Class(id2)));
  }

  SECTION("TimeBounds, FrameBounds, and other Comparisons") {
    REQUIRE("(x_1 - C_TIME >= 1.0)" == fmt::to_string(x1 - C_TIME{} >= 1.0));
    REQUIRE("(x_2 - C_TIME > 1.0)" == fmt::to_string(x2 - C_TIME{} > 1.0));
    REQUIRE("(f_1 - C_FRAME >= 5)" == fmt::to_string(f1 - C_FRAME{} >= 5));
    REQUIRE("(f_2 - C_FRAME > 5)" == fmt::to_string(f2 - C_FRAME{} > 5));
    REQUIRE("(id_1 == id_1)" == fmt::to_string(id1 == id1));
    REQUIRE("(id_1 != id_2)" == fmt::to_string(id1 != id2));
    REQUIRE("(Class(id_1) == Class(id_2))" == fmt::to_string(Class(id1) == Class(id2)));
    REQUIRE("(Class(id_1) != Class(id_2))" == fmt::to_string(Class(id1) != Class(id2)));
    REQUIRE("(Prob(id_1) >= Prob(id_2))" == fmt::to_string(Prob(id1) >= Prob(id2)));
    REQUIRE("(Prob(id_1) > Prob(id_2))" == fmt::to_string(Prob(id1) > Prob(id2)));
    REQUIRE("(Prob(id_1) <= Prob(id_2))" == fmt::to_string(Prob(id1) <= Prob(id2)));
    REQUIRE("(Prob(id_1) < Prob(id_2))" == fmt::to_string(Prob(id1) < Prob(id2)));
  }

  SECTION("Objects and quantifiers over them") {
    REQUIRE("EXISTS {id_1, id_2, id_3}" == fmt::to_string(Exists{{"1"}, {"2"}, {"3"}}));
    REQUIRE("FORALL {id_1, id_2, id_3}" == fmt::to_string(Forall{{"1"}, {"2"}, {"3"}}));
  }

  SECTION("Pinned variables") {
    REQUIRE("{x_1, f_1}" == fmt::to_string(Pin{Var_x{"1"}, Var_f{"1"}}));
    REQUIRE("{_, f_1}" == fmt::to_string(Pin{Var_f{"1"}}));
    REQUIRE("{x_1, _}" == fmt::to_string(Pin{Var_x{"1"}}));
    REQUIRE(
        "{x_1, _} . (x_1 - C_TIME > 10.0)" ==
        fmt::to_string(Pin{Var_x{"1"}}.dot(x1 - C_TIME{} > 10.0)));
  }

  SECTION("Quantifiers over objects pinned at frames") {
    REQUIRE(
        "EXISTS {id_1, id_2} @ {x_1, f_1}" ==
        fmt::to_string(Exists{{"1"}, {"2"}}.at(Pin{Var_x{"1"}, Var_f{"1"}})));
  }

  SECTION("Time bounds over frames and time") {
    REQUIRE("(x_1 - C_TIME >= 2.0)" == fmt::to_string(Var_x{"1"} - C_TIME{} >= 2.0));

    {
      auto [x, f] = std::make_tuple(Var_x{"1"}, Var_f{"1"});
      auto phi    = Pin{x, f}.dot(x - C_TIME{} >= 2.0);
      REQUIRE("{x_1, f_1} . (x_1 - C_TIME >= 2.0)" == fmt::to_string(phi));
    }

    {
      auto [x, f] = std::make_tuple(Var_x{"1"}, Var_f{"1"});
      auto phi    = Pin{x, f}.dot((x - C_TIME{} >= 2.0) >> Const{true});
      REQUIRE("{x_1, f_1} . (~(x_1 - C_TIME >= 2.0) | true)" == fmt::to_string(phi));
    }
  }
}
