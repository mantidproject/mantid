//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Gaussian.h"
#include "MantidAPI/FunctionFactory.h"
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

    for (size_t i = 0; i < nData; i++) {
        double diff=xValues[i]-peakCentre;
        out[i] = height*exp(-0.5*diff*diff*weight);
    }
}

void Gaussian::functionDerivLocal(Jacobian* out, const double* xValues, const size_t nData)
{
    const double& height = getParameter("Height");
    const double& peakCentre = getParameter("PeakCentre");
    const double& weight = pow(1/getParameter("Sigma"),2);

    for (size_t i = 0; i < nData; i++) {
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

    for (size_t i = 0; i < nData; i++) {
        double diff = xValues[i]-peakCentre;
        double e = exp(-0.5*diff*diff*weight);
        out->set(i,0, e);
        out->set(i,1, diff*height*e*weight);
        out->set(i,2, diff*diff*height*e/(sigma*sigma*sigma));  // note this is derivative with respect to sigma not weight
    }
}

void Gaussian::setActiveParameter(size_t i,double value)
{
  if ( !isActive(i) )
  {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Sigma") 
    setParameter(i,sqrt(fabs(1./value)),false);
  else
    setParameter(i,value,false);
}

double Gaussian::activeParameter(size_t i)const
{
  if ( !isActive(i) )
  {
    throw std::runtime_error("Attempt to use an inactive parameter");
  }
  if (parameterName(i) == "Sigma") 
    return 1./pow(getParameter(i),2);
  else
    return getParameter(i);
}


} // namespace CurveFitting
} // namespace Mantid
