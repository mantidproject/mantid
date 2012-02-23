//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/GausOsc.h"
#include <cmath>


namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(GausOsc)

void GausOsc::init()
{
   declareParameter("A", 5.0); 
   declareParameter("Sigma", 0.2);
   declareParameter("Frequency", 0.1); 
   declareParameter("Phi", 0.0);
}


void GausOsc::functionMW(double* out, const double* xValues, const size_t nData)const
{
  const double& A = getParameter("A"); 
  const double& G = getParameter("Sigma");
  const double& gf = getParameter("Frequency"); 
  const double& gphi = getParameter("Phi"); 

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    out[i] = A*exp(-G*G*x*x)*cos(2*M_PI*gf*x +gphi);
  } 
}

void GausOsc::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
  calNumericalDeriv(out, xValues, nData);
}


} // namespace CurveFitting
} // namespace Mantid
