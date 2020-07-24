#ifndef __PERCEMON_MONITORING_HPP__
#define __PERCEMON_MONITORING_HPP__

#include "percemon/ast.hpp"

namespace percemon {
namespace monitoring {

/**
 * Returns if the formula is purely past-time. Only place where this can be true is in
 * the TimeBound and FrameBound constraints.
 *
 * @todo This can be enforced in the TimeBound and FrameBound constructors.
 */
// bool is_past_time(const percemon::ast::Expr& expr);

/**
 * Get the horizon for a formula in number of frames.  This function will throw an
 * exception if there are TimeBound constraints in the formula.
 */
size_t get_horizon(const percemon::ast::Expr& expr);

/**
 * Get the horizon for a formula in number of frames. This uses the frames per second of
 * the stream to convert TimeBound constraints to FrameBound constraints.
 */
size_t get_horizon(const percemon::ast::Expr& expr, double fps);

} // namespace monitoring
} // namespace percemon

#endif /* end of include guard: __PERCEMON_MONITORING_HPP__ */
