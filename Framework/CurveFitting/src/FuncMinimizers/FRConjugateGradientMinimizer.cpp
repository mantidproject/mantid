//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FuncMinimizers/FRConjugateGradientMinimizer.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Logger.h"

namespace Mantid {
namespace CurveFitting {
namespace FuncMinimisers {
namespace {
/// static logger
Kernel::Logger g_log("FRConjugateGradientMinimizer");
} // namespace

// clang-format off
///@cond nodoc
DECLARE_FUNCMINIMIZER(FRConjugateGradientMinimizer,Conjugate gradient (Fletcher-Reeves imp.))
///@endcond
// clang-format on

/// Return a concrete type to initialize m_gslSolver
/// gsl_multimin_fdfminimizer_vector_bfgs2
const gsl_multimin_fdfminimizer_type *
FRConjugateGradientMinimizer::getGSLMinimizerType() {
  return gsl_multimin_fdfminimizer_conjugate_fr;
}

} // namespace FuncMinimisers
} // namespace CurveFitting
} // namespace Mantid
