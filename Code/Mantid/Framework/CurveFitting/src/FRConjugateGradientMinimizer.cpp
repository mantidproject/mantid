//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/FRConjugateGradientMinimizer.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace CurveFitting
{
///@cond nodoc
DECLARE_FUNCMINIMIZER(FRConjugateGradientMinimizer,Conjugate gradient (Fletcher-Reeves imp.))
///@endcond

// Get a reference to the logger
Kernel::Logger& FRConjugateGradientMinimizer::g_log = Kernel::Logger::get("FRConjugateGradientMinimizer");

/// Return a concrete type to initialize m_gslSolver gsl_multimin_fdfminimizer_vector_bfgs2
const gsl_multimin_fdfminimizer_type* FRConjugateGradientMinimizer::getGSLMinimizerType()
{
  return gsl_multimin_fdfminimizer_conjugate_fr;
}

} // namespace CurveFitting
} // namespace Mantid
