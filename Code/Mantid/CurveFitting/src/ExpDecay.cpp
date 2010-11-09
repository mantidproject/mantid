//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ExpDecay.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ExpDecay)

ExpDecay::ExpDecay()
{
  declareParameter("h", 1.0);
  declareParameter("t", 1.0);
}


void ExpDecay::function(double* out, const double* xValues, const int& nData)const
{
    const double& h = getParameter("h");
    const double& t = getParameter("t");

    for (int i = 0; i < nData; i++) 
    {
        out[i] = h*exp( -(xValues[i])/t );
    }
}

void ExpDecay::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    const double& h = getParameter("h");
    const double& t = getParameter("t");

    for (int i = 0; i < nData; i++) {
        double x = xValues[i];
        double e = exp( -x/t );
        out->set(i,0, e);
        out->set(i,1, h*e*x/t/t);
    }

}


} // namespace CurveFitting
} // namespace Mantid
