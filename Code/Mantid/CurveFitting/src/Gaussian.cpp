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
  declareParameter("Sigma", 1.0);
}


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

void Gaussian::calJacobianForCovariance(Jacobian* out, const double* xValues, const int& nData)
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& sigma = getParameter("Sigma");

    for (int i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double weight = 1/(sigma*sigma);
        double e = exp(-0.5*diff*diff*weight);
        out->set(i,0, e);
        out->set(i,1, diff*height*e*weight);
        out->set(i,2, diff*diff*height*e/(sigma*sigma*sigma));
    }
}

void Gaussian::setActiveParameter(int i,double value)
{
  int j = indexOfActive(i);

  if (nameOfActive(j) == "Sigma") 
    parameter(j) = sqrt(1./value);
  else
    parameter(j) = value;
}

double Gaussian::activeParameter(int i)
{
  int j = indexOfActive(i);

  if (nameOfActive(j) == "Sigma") 
    return 1./pow(parameter(j),2);
  else
    return parameter(j);
}


} // namespace CurveFitting
} // namespace Mantid
