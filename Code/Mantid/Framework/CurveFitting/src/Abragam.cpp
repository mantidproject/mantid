//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Abragam.h"
#include "MantidAPI//FunctionFactory.h"
#include <cmath>


namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(Abragam)

void Abragam::init()
{
  declareParameter("A", 0.2); 
  declareParameter("Omega", 0.5); 
  declareParameter("Phi", 0); 
  declareParameter("Sigma", 1); 
  declareParameter("Tau",1); 
}


void Abragam::function1D(double* out, const double* xValues, const size_t nData)const
{
  const double& A = getParameter("A"); 
  const double& w = getParameter("Omega"); 
  const double& phi = getParameter("Phi"); 
  const double& sig = getParameter("Sigma");   
  const double& t = getParameter("Tau"); 

  for (size_t i = 0; i < nData; i++) {
    double A1=A*cos(w*xValues[i]+phi); 
    double A2=-(sig*sig*t*t)*(exp(-xValues[i]/t)-1+(xValues[i]/t)); 
    double A3=exp(A2); 

    out[i] = A1*A3; 
  } 

}

void Abragam::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  calNumericalDeriv(domain,jacobian);
}

void Abragam::setActiveParameter(size_t i,double value)
{
  size_t j = i;

  if (parameterName(j) == "Sigma"  ) 
    setParameter(j,fabs(value),false);  // Make sigma positive
  else if (parameterName(j) == "Phi")
  {
    double a = fmod(value, 2*M_PI); // Put angle in range of 0 to 360 degrees
    if( a<0 ) a += 2*M_PI; 
    setParameter(j,a,false);
  }
  else
    setParameter(j,value,false);
}


} // namespace CurveFitting
} // namespace Mantid
