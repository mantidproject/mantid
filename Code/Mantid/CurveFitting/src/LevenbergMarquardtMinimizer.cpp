//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LevenbergMarquardtMinimizer.h"
#include <gsl/gsl_errno.h>
#include <gsl/gsl_fit.h>
#include "MantidKernel/Exception.h"

namespace Mantid
{
namespace CurveFitting
{

/// Get name of minimizer
std::string LevenbergMarquardtMinimizer::name()const
{
  throw Kernel::Exception::NotImplementedError("Not implemented yet");
  return std::string("Not implemented yet");
}

/// Perform iteration with minimizer
void LevenbergMarquardtMinimizer::iterate() 
{
  throw Kernel::Exception::NotImplementedError("Not implemented yet");
}


} // namespace CurveFitting
} // namespace Mantid
