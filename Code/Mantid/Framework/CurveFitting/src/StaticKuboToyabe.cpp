//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/StaticKuboToyabe.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(StaticKuboToyabe)

void StaticKuboToyabe::init()
{
  declareParameter("A", 0.2);
  declareParameter("Delta", 0.2);
}


void StaticKuboToyabe::functionMW(double* out, const double* xValues, const size_t nData)const
{
  const double& A = getParameter("A"); 
  const double& G = getParameter("Delta"); 


  for (int i = 0; i < nData; i++) {  
    out[i] = A*(exp(-pow(G*xValues[i],2)/2)*(1-pow(G*xValues[i],2))*2.0/3 + 1.0/3); 
  } 

}

void StaticKuboToyabe::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
  calNumericalDeriv(out, xValues, nData);
}


} // namespace CurveFitting
} // namespace Mantid
