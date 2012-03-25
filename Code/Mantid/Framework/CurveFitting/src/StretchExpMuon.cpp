//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/StretchExpMuon.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(StretchExpMuon)

void StretchExpMuon::init()
{
  declareParameter("A", 0.2);
  declareParameter("Lambda", 0.2);
  declareParameter("Beta",0.2);
}


void StretchExpMuon::functionMW(double* out, const double* xValues, const size_t nData)const
{
  const double& A = getParameter("A");
  const double& G = getParameter("Lambda");
  const double& b = getParameter("Beta");


  for (size_t i = 0; i < nData; i++) {
    out[i] = A*exp(-pow(G*xValues[i],b));
  }
}
void StretchExpMuon::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
  calNumericalDeriv(out, xValues, nData);
}




} // namespace CurveFitting
} // namespace Mantid


