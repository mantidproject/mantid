//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Quadratic.h"

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

void Quadratic::init()
{
  declareParameter("A0", 0.0);
  declareParameter("A1", 0.0);
  declareParameter("A2", 0.0);
}


void Quadratic::function(double* out, const double* xValues, const int& nData)
{
    const double& a0 = getParameter("A0");
    const double& a1 = getParameter("A1");
    const double& a2 = getParameter("A2");

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i] - xValues[0];
        out[i] = a0+a1*diff+a2*diff*diff;
    }
}

void Quadratic::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    for (int i = 0; i < nData; i++) {
        double diff = xValues[i] - xValues[0];
        out->set(i,0, 1);
        out->set(i,1, diff);
        out->set(i,2, diff*diff);
    }
}


} // namespace CurveFitting
} // namespace Mantid
