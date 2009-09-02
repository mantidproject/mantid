//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/IkedaCarpenterPV1D.h"
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_multifit_nlin.h>

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(IkedaCarpenterPV1D)

using namespace Kernel;

void IkedaCarpenterPV1D::declareParameters()
{
  BoundedValidator<double> *positiveDouble = new BoundedValidator<double>();
  NullValidator<double> *noValidation = new NullValidator<double>; //the null validator always returns valid, there is no validation
  declareProperty("I", 0.0, noValidation,
    "Magnitude of peak (default 0)", Direction::InOut);
  declareProperty("Alpha0",1.6, noValidation->clone(),
    "Used to model fast decay constant (default 1.6)", Direction::InOut);
  declareProperty("Alpha1",1.5, noValidation->clone(),
    "Used to model fast decay constant (default 1.5)", Direction::InOut);
  declareProperty("Beta0",31.9, positiveDouble,
    "Inverse of slow decay constant  (default 31.9)", Direction::InOut);
  declareProperty("Kappa",46.0, positiveDouble->clone(),
    "Controls contribution of slow decay term   (default 46.0)", Direction::InOut);
  declareProperty("SigmaSquared",1.0, positiveDouble->clone(),
    "standard deviation squared (Guassian broadening) (default 1.0)", Direction::InOut);
  declareProperty("Gamma",1.0, positiveDouble->clone(),
    "Lorentzian broadening  (default 1.0)", Direction::InOut);

  BoundedValidator<double> *percentageValue = new BoundedValidator<double>(0.0,1.0);
  declareProperty("Eta",0.0, percentageValue,
    "Mixing parameter (percentage in terms of Lorentzian part) (default 0.0)", Direction::InOut);

  declareProperty("X0",0.0,  noValidation->clone(),
    "Peak position (default 0)", Direction::InOut);
  declareProperty("BG", 0.0, noValidation->clone(),
    "Constant background value (default 0)", Direction::InOut);
}



void IkedaCarpenterPV1D::function(const double* in, double* out, const double* xValues, const double* yValues, const double* yErrors, const int& nData)
{
    const double& I = in[0];
    const double& alpha0 = in[1];
    const double& alpha1 = in[2];
    const double& beta0 = in[3];
    const double& kappa = in[4];
    const double& SigmaSquared = in[5];
    const double& Gamma = in[6];
    const double& Eta = in[7];
    const double& X0 = in[8];
    const double& BG = in[9];

    const double alpha = 1.0 / (alpha0+alpha1); // assume wavelength = 1
    const double beta = 1/beta0;

    const double R = exp(-81.799/kappa);  // assume wavelength = 1
    const double k = 0.05;
    const double a_minus = alpha*(1-k);
    const double a_plus = alpha*(1+k);
    const double x=a_minus-beta;
    const double y=alpha-beta;
    const double z=a_plus-beta;    

    const double Nu=1-R*a_minus/x;
    const double Nv=1-R*a_plus/z;
    const double Ns=-2*(1-R*alpha/y);
    const double Nr=2*R*alpha*alpha*beta*k*k/(x*y*z);

    double u,v,s,r;
    double yu, yv, ys, yr;

    const double someConst = 1/sqrt(2.0*SigmaSquared);

    for (int i = 0; i < nData; i++) {
        double diff=xValues[i]-X0;

        u=a_minus*(a_minus*SigmaSquared-2*diff)/2.0;
        v=a_plus*(a_plus*SigmaSquared-2*diff)/2.0;
        s=alpha*(alpha*SigmaSquared-2*diff)/2.0;
        r=beta*(beta*SigmaSquared-2*diff)/2.0;

        yu = (a_minus*SigmaSquared-diff)*someConst;
        yv = (a_plus*SigmaSquared-diff)*someConst;
        ys = (alpha*SigmaSquared-diff)*someConst;
        yr = (beta*SigmaSquared-diff)*someConst;



        double Yi = I*( Nu*exp(u)*gsl_sf_erfc(yu)+Nv*exp(v)*gsl_sf_erfc(yv) + 
                        Ns*exp(s)*gsl_sf_erfc(ys)+Nr*exp(r)*gsl_sf_erfc(yr) ) + BG;
        out[i] = (Yi - yValues[i])/yErrors[i];
    }
}



} // namespace CurveFitting
} // namespace Mantid
