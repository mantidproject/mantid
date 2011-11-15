//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LogNormal.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(LogNormal)

LogNormal::LogNormal()
{
  declareParameter("Height", 1.0);
  declareParameter("PeakCentre", 1.0);
  declareParameter("Sigma", 1.0);
}


void LogNormal::functionMW(double* out, const double* xValues, const size_t nData)const
{
    const double& h = getParameter("Height");
    const double& t = getParameter("PeakCentre");
    const double& b = getParameter("Sigma");

    for (size_t i = 0; i < nData; i++) 
    {
    	double x = xValues[i];
    	if( x == 0.0 ) out[i] = 0.0;
    	else
    	{
    		double c = log(x/t)/b;
    		out[i] = h*(t/x)*exp( -c*c/2 );
    	}
    }
}

void LogNormal::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
    const double& h = getParameter("Height");
    const double& t = getParameter("PeakCentre");
    const double& b = getParameter("Sigma");

    for (size_t i = 0; i < nData; i++) {
        double x = xValues[i];
        double c = log(x/t)/b;
        double e = exp( -c*c/2 );
        out->set(i,0, (t/x)*e);         //partial derivative with respect to Height
        out->set(i,1, h*e*(1+c/b)/x );  //partial derivative with respect to PeakCentre
        out->set(i,2, h*(t/x)*e*c*c/b); //partial derivative with respect to Sigma
    }

}


} // namespace CurveFitting
} // namespace Mantid
