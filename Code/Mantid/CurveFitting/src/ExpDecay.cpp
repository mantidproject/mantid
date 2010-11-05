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
  declareParameter("c", 0.0);
  declareParameter("t", 1.0);
}


void ExpDecay::function(double* out, const double* xValues, const int& nData)const
{
    const double& h = getParameter("h");
    const double& c = getParameter("c");
    const double& t = getParameter("t");

    for (int i = 0; i < nData; i++) 
    {
        out[i] = h*exp( -(xValues[i]-c)/t );
    }
}

void ExpDecay::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    const double& h = getParameter("h");
    const double& c = getParameter("c");
    const double& t = getParameter("t");

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-c;
        double e = exp( -diff/t );
        out->set(i,0, e);
        e *= h/t;
        out->set(i,1, e);
        out->set(i,2, e*diff/t);
    }

}


} // namespace CurveFitting
} // namespace Mantid
