//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/PRConjugateGradientMinimizer.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Logger.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {

// clang-format off
///@cond nodoc
DECLARE_FUNCMINIMIZER(PRConjugateGradientMinimizer,Conjugate gradient (Polak-Ribiere imp.))
///@endcond
// clang-format on

/// Return a concrete type to initialize m_gslSolver
/// gsl_multimin_fdfminimizer_vector_bfgs2
const gsl_multimin_fdfminimizer_type *
PRConjugateGradientMinimizer::getGSLMinimizerType() {
  return gsl_multimin_fdfminimizer_conjugate_pr;
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
