//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DynamicKuboToyabe.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/FunctionFactory.h"
#include <vector>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(DynamicKuboToyabe)

void DynamicKuboToyabe::init()
{
  declareParameter("Asym",  0.2, "Amplitude at time 0");
  declareParameter("Delta", 0.2, "Local field");
  declareParameter("Field", 0.0, "External field");
  declareParameter("Nu",    0.0, "Hopping rate");
}

// Static Zero Field Kubo Toyabe relaxation function
double ZFKT (const double x, const double G){

  const double q = G*G*x*x;
  return (0.3333333333 + 0.6666666667*exp(-0.5*q)*(1-q));
}

// Static non-zero field Kubo Toyabe relaxation function
double HKT (const double x, const double G, const double F) {

  const double q = G*G*x*x;
  const double gm = 2*M_PI*0.01355342; // Muon gyromagnetic ratio * 2 * PI
  
  double w;
  if (F>2*G) {
    // Use F
    w = gm * F;
  } else {
    // Use G
    w = gm * 2 * G;
  }
  
  const double r = G*G/w/w;
  
  double ig;
  if ( x>0 && r>0 ) {
    // Compute integral
    ig = integral(f1,0.0,x,G,w);
  } else {
    // Integral is 0
    ig = 0;
  }
  
  const double ktb=(1-2*r*(1-exp(-q/2)*cos(w*x))+2*r*r*w*ig);
  
  if ( F>2*G ) {
    return ktb;
  } else {
    const double kz = ZFKT(x,G);
    return kz+F/2/G*(ktb-kz);
  }
  
}

// Dynamic Kubo-Toyabe
double getDKT (double t, double G, double v){

  const int tsmax = 656; // Length of the time axis, 32 us of valid data
  const double eps = 0.05; // Bin width for calculations

  static double oldG=-1., oldV=-1.;
  static std::vector<double> gStat(tsmax), gDyn(tsmax);


  if ( (G != oldG) || (v != oldV) ){

    // If G or v have changed with respect to the 
    // previous call, we need to re-do the computations


    if ( G != oldG ){

      // But we only need to
      // re-compute gStat if G has changed

      // Generate static Kubo-Toyabe
      for (int k=0; k<tsmax; k++){
        gStat[k]= ZFKT(k*eps,G);
      }
      // Store new G value
      oldG =G;
    }

    // Store new v value
    oldV =v;

    double hop = v*eps;

    // Generate dynamic Kubo Toyabe
    for (int k=0; k<tsmax; k++){
      double y=gStat[k];
      for (int j=k-1; j>0; j--){
        y=y*(1-hop)+hop*gDyn[k-j]*gStat[j];
      }
      gDyn[k]=y;
    }
  }

  // Interpolate table 
  // If beyond end, extrapolate
  int x=int(fabs(t)/eps);
  if (x>tsmax-2)
    x = tsmax-2;
  double xe=(fabs(t)/eps)-x;
  return gDyn[x]*(1-xe)+xe*gDyn[x+1];

}

// Dynamic Kubo Toyabe function
void DynamicKuboToyabe::function1D(double* out, const double* xValues, const size_t nData)const
{
  const double& A = getParameter("Asym");
  const double& G = fabs(getParameter("Delta"));
  const double& F = fabs(getParameter("Field"));
  const double& v = fabs(getParameter("Nu"));


  // Zero hopping rate
  if (v == 0.0) {

    // Zero external field
    if ( F == 0.0 ){
      for (size_t i = 0; i < nData; i++) {
        out[i] = A*ZFKT(xValues[i],G);
      }
    }
    // Non-zero external field
    else{
      throw std::runtime_error("HKT() not implemented yet");
    }
  } 

  // Non-zero hopping rate
  else {

    if ( F==0.0 ) {

      for (size_t i = 0; i<nData; i++){
        out[i] = A*getDKT(xValues[i],G,v);
      }

    } else {

      // Non-zero field
      throw std::runtime_error("HKT() not implemented yet");
    }

  } // else hopping rate != 0


}



void DynamicKuboToyabe::functionDeriv(const API::FunctionDomain& domain, API::Jacobian& jacobian)
{
  calNumericalDeriv(domain, jacobian);
}

void DynamicKuboToyabe::functionDeriv1D(API::Jacobian* , const double* , const size_t )
{
  throw Mantid::Kernel::Exception::NotImplementedError("functionDerivLocal is not implemented for DynamicKuboToyabe.");
}

void DynamicKuboToyabe::setActiveParameter(size_t i, double value) {

  setParameter( i, fabs(value), false);

}

} // namespace CurveFitting
} // namespace Mantid
