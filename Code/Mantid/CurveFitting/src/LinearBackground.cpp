//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/LinearBackground.h"

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

void LinearBackground::init()
{
  declareParameter("A0", 0.0);
  declareParameter("A1", 0.0);
}


void LinearBackground::function(double* out, const double* xValues, const int& nData)
{
    const double& a0 = getParameter("A0");
    const double& a1 = getParameter("A1");

    for (int i = 0; i < nData; i++) {
        out[i] = a0+a1*(xValues[i] - xValues[0]);
    }
}

void LinearBackground::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    for (int i = 0; i < nData; i++) {
        out->set(i,0, 1);
        out->set(i,1, xValues[i] - xValues[0]);
    }
}


} // namespace CurveFitting
} // namespace Mantid
