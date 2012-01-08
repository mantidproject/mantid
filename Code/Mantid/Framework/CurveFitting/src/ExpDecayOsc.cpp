//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/ExpDecayOsc.h"
#include <cmath>

const double PI = 3.1415926536;

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ExpDecayOsc)

void ExpDecayOsc::init()
{
   declareParameter("A", 0.2); 
   declareParameter("Lambda", 0.2); 
   declareParameter("Frequency", 0.1); 
   declareParameter("Phi", 0.0); 
}


void ExpDecayOsc::functionMW(double* out, const double* xValues, const size_t nData)const
{
  const double& gA0 = getParameter("A"); 
  const double& gs = getParameter("Lambda"); 
  const double& gf = getParameter("Frequency"); 
  const double& gphi = getParameter("Phi"); 

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    out[i] = gA0*exp(-gs*x)*cos(2*PI*gf*x +gphi);
  } 
}

void ExpDecayOsc::functionDerivMW(Jacobian* out, const double* xValues, const size_t nData)
{
    const double& gA0 = getParameter("A");
    const double& gs = getParameter("Lambda");
    const double& gf = getParameter("Frequency"); 
    const double& gphi = getParameter("Phi"); 

    for (size_t i = 0; i < nData; i++) {
        double x = xValues[i];
        double e = exp( -gs*x );
        double c = cos(2*PI*gf*x +gphi);
        double s = sin(2*PI*gf*x +gphi);
        out->set(i,0, e*c);            //derivative w.r.t. A (gA0)
        out->set(i,1, gA0*x*e*c);      //derivative w.r.t  Lambda (gs)
        out->set(i,2, gA0*e*2*PI*x*s); // derivate w.r.t. Frequency (gf)
        out->set(i,3, gA0*e*s);        // detivative w.r.t Phi (gphi) 
    }
}


} // namespace CurveFitting
} // namespace Mantid
