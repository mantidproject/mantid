//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/SteepestDescentMinimizer.h"
#include "MantidAPI/CostFunctionFactory.h"
#include "MantidKernel/Logger.h"

namespace Mantid
{
namespace CurveFitting
{
DECLARE_FUNCMINIMIZER(SteepestDescentMinimizer,SteepestDescent)

// Get a reference to the logger
Kernel::Logger& SteepestDescentMinimizer::g_log = Kernel::Logger::get("SteepestDescentMinimizer");


/// Return a concrete type to initialize m_gslSolver gsl_multimin_fdfminimizer_vector_bfgs2
const gsl_multimin_fdfminimizer_type* SteepestDescentMinimizer::getGSLMinimizerType()
{
  return  gsl_multimin_fdfminimizer_steepest_descent;
}

} // namespace CurveFitting
} // namespace Mantid
