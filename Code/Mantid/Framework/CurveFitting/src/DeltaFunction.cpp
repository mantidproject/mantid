//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DeltaFunction.h"
#include <algorithm>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(DeltaFunction)

DeltaFunction::DeltaFunction()
{
  declareParameter("Height", 1.0);
  //declareParameter("Centre", 0.0);
}


void DeltaFunction::function(double* out, const double* xValues, const size_t nData)const
{
  UNUSED_ARG(xValues);
  std::fill(out,out+nData,0);
}

void DeltaFunction::functionDeriv(Jacobian* out, const double* xValues, const size_t nData)
{
  UNUSED_ARG(out); UNUSED_ARG(xValues); UNUSED_ARG(nData);
  std::runtime_error("Cannot compute derivative of a delta function");
}

} // namespace CurveFitting
} // namespace Mantid
