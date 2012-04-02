//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BFGS_Minimizer.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace CurveFitting
{
DECLARE_FUNCMINIMIZER(BFGS_Minimizer,BFGS)

// Get a reference to the logger
Kernel::Logger& BFGS_Minimizer::g_log = Kernel::Logger::get("BFGS_Minimizer");


/// Return a concrete type to initialize m_gslSolver gsl_multimin_fdfminimizer_vector_bfgs2
const gsl_multimin_fdfminimizer_type* BFGS_Minimizer::getGSLMinimizerType()
{
  return gsl_multimin_fdfminimizer_vector_bfgs2;
}

} // namespace CurveFitting
} // namespace Mantid
