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

DECLARE_FUNCTION(LinearBackground)

void LinearBackground::init()
{
  declareParameter("A0", 0.0);
  declareParameter("A1", 0.0);
} 


void LinearBackground::function(double* out, const double* xValues, const int& nData)const
{
    const double& a0 = getParameter("A0");
    const double& a1 = getParameter("A1");

    for (int i = 0; i < nData; i++) {
        out[i] = a0+a1*xValues[i];
    }
}

void LinearBackground::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    for (int i = 0; i < nData; i++) {
        out->set(i,0, 1);
        out->set(i,1, xValues[i]);
    }
}

/**
 * Do linear fit to the data in X and Y
 * @param X :: Vector with x-values
 * @param Y :: Vector with y-values
 */
void LinearBackground::fit(const std::vector<double>& X,const std::vector<double>& Y)
{
  if (X.size() != Y.size())
  {
    throw std::runtime_error("Background fit: different array sizes");
  }
  int n = X.size();
  if (n == 0)
  {
    setParameter("A0",0);
    setParameter("A1",0);
    return;
  }
  else if (n == 1)
  {
    setParameter("A0",Y[0]);
    setParameter("A1",0);
    return;
  }
  double x_mean = 0;
  double y_mean = 0;
  double x2_mean = 0;
  double xy_mean = 0;
  for(int i = 0; i < n; i++)
  {
    double x = X[i];
    double y = Y[i];
    x_mean += x;
    y_mean += y;
    x2_mean += x*x;
    xy_mean += x*y;
  }
  x_mean /= n;
  y_mean /= n;
  x2_mean /= n;
  xy_mean /= n;

  double a1 = (xy_mean - x_mean*y_mean)/(x2_mean-x_mean*x_mean);
  double a0 = y_mean - a1*x_mean;

  setParameter("A0",a0);
  setParameter("A1",a1);
}


} // namespace CurveFitting
} // namespace Mantid
