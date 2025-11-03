#include "percemon/evaluation.hpp"

#include <nanobind/nanobind.h>
#include <nanobind/stl/bind_map.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/pair.h>
#include <nanobind/stl/shared_ptr.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/unique_ptr.h>
#include <nanobind/stl/variant.h>
#include <nanobind/stl/vector.h>

#include <vector>

namespace nb = nanobind;
using namespace percemon::monitoring; // NOLINT(google-build-using-namespace)
using namespace percemon::datastream; // NOLINT(google-build-using-namespace)

// NOLINTBEGIN(misc-redundant-expression)
// NOLINTNEXTLINE(misc-use-internal-linkage)
void init_evaluation_bindings(nb::module_& m) {
  // =====================================================================
  // BooleanEvaluator
  // =====================================================================

  nb::class_<BooleanEvaluator>(m, "BooleanEvaluator")
      .def(nb::init<>())
      .def(
          "evaluate",
          [](const BooleanEvaluator& evaluator,
             const percemon::stql::Expr& formula,
             const Frame& current_frame,
             const std::vector<Frame>& history,
             const std::vector<Frame>& horizon) -> bool {
            // Convert std::vector to std::deque for the evaluation
            std::deque<Frame> hist(history.begin(), history.end());
            std::deque<Frame> horiz(horizon.begin(), horizon.end());
            return evaluator.evaluate(formula, current_frame, hist, horiz);
          },
          nb::arg("formula"),
          nb::arg("current_frame"),
          nb::arg("history"),
          nb::arg("horizon"),
          "Evaluate STQL formula on a frame with history and horizon buffers");
}
// NOLINTEND(misc-redundant-expression)
