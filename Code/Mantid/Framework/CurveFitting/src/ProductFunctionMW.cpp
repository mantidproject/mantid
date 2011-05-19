// To suppress STL warnings
#define _SCL_SECURE_NO_WARNINGS
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ProductFunctionMW.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/DeltaFunction.h"
#include <cmath>
#include <algorithm>
#include <functional>
#include "MantidAPI/ParameterTie.h"
#include "MantidAPI/IConstraint.h"

#include <gsl/gsl_errno.h>
#include <gsl/gsl_fft_real.h>
#include <gsl/gsl_fft_halfcomplex.h>

#include <sstream>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ProductFunctionMW)

void ProductFunctionMW::function(double* out, const double* xValues, const size_t nData)const
{
  if (nData == 0) return;
  boost::shared_array<double> tmpOut(new double[nData]);
  for(int i=0;i<nFunctions();i++)
  {
    IFunctionMW* fun = dynamic_cast<IFunctionMW*>(getFunction(i));
    if (i == 0)
      fun->function(out,xValues,nData);
    else
    {
      fun->function(tmpOut.get(),xValues,nData);
      std::transform(out,out+nData,tmpOut.get(),out,std::multiplies<double>());
    }
  }
}

void ProductFunctionMW::functionDeriv(Jacobian* out, const double* xValues, const size_t nData)
{
  calNumericalDeriv(out, xValues, nData);
}


} // namespace CurveFitting
} // namespace Mantid
