//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FRConjugateGradientMinimizer.h"

#include "MantidAPI/CostFunctionFactory.h"
#include "MantidAPI/FuncMinimizerFactory.h"

#include "MantidKernel/Logger.h"

namespace Mantid {
namespace CurveFitting {
namespace {
/// static logger
Kernel::Logger g_log("FRConjugateGradientMinimizer");
}

///@cond nodoc
DECLARE_FUNCMINIMIZER(FRConjugateGradientMinimizer,
                      Conjugate gradient(Fletcher - Reeves imp.))
///@endcond

/// Return a concrete type to initialize m_gslSolver
/// gsl_multimin_fdfminimizer_vector_bfgs2
const gsl_multimin_fdfminimizer_type *
FRConjugateGradientMinimizer::getGSLMinimizerType() {
  return gsl_multimin_fdfminimizer_conjugate_fr;
}

} // namespace CurveFitting
} // namespace Mantid
