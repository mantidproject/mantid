//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Gaussian.h"
#include <cmath>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Gaussian)

void Gaussian::init()
{
  declareParameter("Height", 0.0);
  declareParameter("PeakCentre", 0.0);
  declareParameter("Sigma", 0.0);
}


void Gaussian::functionLocal(double* out, const double* xValues, const size_t nData)const
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& weight = pow(1/getParameter("Sigma"),2);

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        out[i] = height*exp(-0.5*diff*diff*weight);
    }
}

void Gaussian::functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& weight = pow(1/getParameter("Sigma"),2);

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight);
        out->set(i,0, e);
        out->set(i,1, diff*height*e*weight);
        out->set(i,2, -0.5*diff*diff*height*e);  // derivative with respect to weight not sigma
    }
}

void Gaussian::calJacobianForCovariance(Jacobian* out, const double* xValues, const size_t nData)
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& sigma = getParameter("Sigma");

    double weight = 1/(sigma*sigma);

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight);
        out->set(i,0, e);
        out->set(i,1, diff*height*e*weight);
        out->set(i,2, diff*diff*height*e/(sigma*sigma*sigma));  // note this is derivative with respect to sigma not weight
    }
}

void Gaussian::setActiveParameter(int i,double value)
{
  int j = indexOfActive(i);

  if (parameterName(j) == "Sigma") 
    setParameter(j,sqrt(1./value),false);
  else
    setParameter(j,value,false);
}

double Gaussian::activeParameter(int i)const
{
  int j = indexOfActive(i);

  if (parameterName(j) == "Sigma") 
    return 1./pow(getParameter(j),2);
  else
    return getParameter(j);
}


} // namespace CurveFitting
} // namespace Mantid
