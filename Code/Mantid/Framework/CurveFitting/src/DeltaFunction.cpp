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


void DeltaFunction::function(double* out, const double* xValues, const int& nData)const
{
  std::fill(out,out+nData,0);
}

void DeltaFunction::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
  (void) out; (void) xValues; (void) nData; //Avoid compiler warning
  std::runtime_error("Cannot compute derivative of a delta function");
}

} // namespace CurveFitting
} // namespace Mantid
