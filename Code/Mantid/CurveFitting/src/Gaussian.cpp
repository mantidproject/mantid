//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Gaussian.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory

using namespace Kernel;
using namespace API;

void Gaussian::init()
{
  declareParameter("Height", 0.0);
  declareParameter("PeakCentre", 0.0);
  declareParameter("Sigma", 1.0);
}


/*

void Gaussian::modifyStartOfRange(double& startX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  startX = peak_val-(6*sigma);
}

void Gaussian::modifyEndOfRange(double& endX) 
{
  const double peak_val = getProperty("PeakCentre");
  const double sigma = getProperty("Sigma");

  endX = peak_val+(6*sigma);
}

*/

void Gaussian::function(double* out, const double* xValues, const int& nData)
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& weight = pow(1/getParameter("Sigma"),2);

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        out[i] = height*exp(-0.5*diff*diff*weight);
    }
}

void Gaussian::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& weight = pow(1/getParameter("Sigma"),2);

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight);
        out->set(i,0, e);
        out->set(i,1, diff*height*e*weight);
        out->set(i,2, -0.5*diff*diff*height*e);
    }
}

} // namespace CurveFitting
} // namespace Mantid
