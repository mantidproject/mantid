//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ExpDecayMuon.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ExpDecayMuon)

void ExpDecayMuon::init()
{
  declareParameter("A", 0.2);
  declareParameter("Lambda", 0.2);
}


void ExpDecayMuon::functionMW(double* out, const double* xValues, const size_t nData)const
{
    const double& gA0 = getParameter("A");
    const double& gs = getParameter("Lambda");

    for (size_t i = 0; i < nData; i++) 
    {
        out[i] = gA0*exp( -gs*(xValues[i]) );
    }
}

void ExpDecayMuon::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
  // Use numerical derivates, because attempt to use analytic derivatives failed
    calNumericalDeriv(out, xValues, nData);
}


} // namespace CurveFitting
} // namespace Mantid
