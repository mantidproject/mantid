//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/DynamicKuboToyabe.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/FunctionFactory.h"
#include <vector>

#define EPS 1e-6
#define JMAX 14
#define JMAXP (JMAX+1)
#define K 5
#define NR_END 1
#define FREE_ARG char*

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(DynamicKuboToyabe)

// ** MODIFY THIS **
// Here specify/declare the parameters of your Fit Function
// 
// declareParameter takes three arguments:
//
//   1st: The name of the parameter
//   2nd: The default (initial) value of the parameter
//   3rd: A description of the parameter (optional)
//
void DynamicKuboToyabe::init()
{
  declareParameter("Asym",  0.2, "Amplitude at time 0");
  declareParameter("Delta", 0.2, "Local field");
  declareParameter("Field", 0.0, "External field");
  declareParameter("Nu",    0.0, "Hopping rate");
}

double getDKT (double t, double sig, double nu){

#define tsmax 656 // 16 us of valid data
#define stk 25
#define eps 0.05 // bin for calculations


  int k,j, xi;
  static double gs[tsmax+1], gd[tsmax+1];

  double y, tt, hop, xe;
  static double oldsig=-1., oldnu=-1.;


  if ( (sig == oldsig) && (nu == oldnu) ){
    // Re-use previous computation
    xi=int(fabs(t)/eps);
    if (xi>tsmax-2)
      xi = tsmax-2;
    xe=(fabs(t)/eps)-xi;
    return gd[xi+1]*(1-xe)+xe*gd[xi+2];
  }

  oldsig=sig;
  oldnu =nu;

  hop = nu*eps;

  // Generate static Kubo-Toyabe
  for (k=1; k<=tsmax; k++){
    tt = (k-1)*eps*sig*(k-1)*eps*sig;
    gs[k]=0.3333333333+0.6666666667*(1-tt)*exp(-tt/2);
  }

  // Generate dynamic f-u-f
  for (k=1; k<=tsmax; k++){
    y=gs[k];
    for (j=k-1; j>=2; j--){
      y=y*(1-hop)+hop*gd[k-j+1]*gs[j];
    }
    gd[k]=y;
  }

  
  // Interpolate table. If beyond end, extrapolate...
  xi=int(abs(t)/eps);
  if (xi>tsmax-2)
    xi = tsmax-2;
  xe=abs(t)/eps-xi;
  return gd[xi+1]*(1-xe)+xe*gd[xi+2];

}

// Zero Field Kubo Toyabe relaxation function
double ZFKT (const double x, const double G){

  const double q = G*G*x*x;
  return (0.3333333333 + 0.6666666667*exp(-0.5*q)*(1-q));
}

// Non-Zero field Kubo Toyabe relaxation function
double HKT (const double x, const double G, const double F) 
{
  throw std::runtime_error("HKT not implemented yet");
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
      for (size_t i = 0; i < nData; i++) {
        out[i] = A*HKT(xValues[i],G,F);
      }
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
      throw std::runtime_error("Not implemented yet");
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
