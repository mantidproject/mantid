//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BackToBackExponential1D.h"
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_multifit_nlin.h>

namespace Mantid
{
namespace CurveFitting
{

// Register the class into the algorithm factory
DECLARE_ALGORITHM(BackToBackExponential1D)

using namespace Kernel;

// Get a reference to the logger
Logger& BackToBackExponential1D::g_log = Logger::get("BackToBackExponential1D");


void BackToBackExponential1D::declareParameters()
{
  NullValidator<double> *noValidation = new NullValidator<double>;                      //the null validator always returns valid, there is no validation
  declareProperty("I", 0.0, noValidation, "Height of the peak (default 0)",
    Direction::InOut);
  declareProperty("a",0.0, noValidation->clone(),
    "Exponential constant of rising part of neutron pulse (default 0)",
    Direction::InOut);
  declareProperty("b", 0.0, noValidation->clone(),
    "Exponential constant of decaying part of neutron pulse (default 0)",
    Direction::InOut);
  declareProperty("x0", 0.0, noValidation->clone(), 
    "Peak position (default 0)", Direction::InOut );
  declareProperty("s", 1.0, noValidation->clone(),
    "Standard deviation of the gaussian part of the peakshape (default 1)",
    Direction::InOut );
  declareProperty("bk", 0.0, noValidation->clone(),
    "Constant background value (default 0)", Direction::InOut);
}



void BackToBackExponential1D::function(double* in, double* out, double* xValues, double* yValues, double* yErrors, int nData)
{
    double I = in[0];
    double a = in[1];
    double b = in[2];
    double x0 = in[3];
    double s = in[4];
    double bk = in[5];

    double s2 = s*s;
    for (size_t i = 0; i < nData; i++) {
      double diff=xValues[i]-x0;
      double Yi = I*(exp(a/2*(a*s2+2*diff))*gsl_sf_erfc((a*s2+diff)/sqrt(2*s2))
                    + exp(b/2*(b*s2-2*diff))*gsl_sf_erfc((b*s2-diff)/sqrt(2*s2)))+bk;
      out[i] = (Yi - yValues[i])/yErrors[i];
    }
}

void BackToBackExponential1D::functionDeriv(double* in, double* out, double* xValues, double* yValues, double* yErrors, int nData)
{
    double I = in[0];
    double a = in[1];
    double b = in[2];
    double x0 = in[3];
    double s = in[4];

    double s2 = s*s;
    int nParam = m_parameterNames.size();
    for (size_t i = 0; i < nData; i++) {
        double diff = xValues[i]-x0;

        double e_a = exp(0.5*a*(a*s2+2*diff));
        double e_b = exp(0.5*b*(b*s2-2*diff));
        double erfc_a = gsl_sf_erfc((a*s2+diff)/sqrt(2*s2));
        double erfc_b = gsl_sf_erfc((b*s2-diff)/sqrt(2*s2));

        // apart from a prefactor terms arising from defivative or argument of erfc's divided by sqrt(2)
        double div_erfc_a = - exp( -(a*s2+diff)*(a*s2+diff)/(2*s2)+0.5*a*(a*s2+2.0*diff) ) * M_SQRT2/M_SQRTPI;
        double div_erfc_b = - exp( -(b*s2-diff)*(b*s2-diff)/(2*s2)+0.5*b*(b*s2-2.0*diff) ) * M_SQRT2/M_SQRTPI;

        out[i*nParam + 0] = (e_a*erfc_a+e_b*erfc_b)/yErrors[i];
        out[i*nParam + 1] = I*( s*div_erfc_a + e_a*(a*s2+diff)*erfc_a )/yErrors[i];
        out[i*nParam + 2] = I*( s*div_erfc_b + e_b*(b*s2-diff)*erfc_b )/yErrors[i];
        out[i*nParam + 3] = I*( (-div_erfc_a+div_erfc_b)/s + b*e_b*erfc_b - a*e_a*erfc_a )/yErrors[i];
        out[i*nParam + 4] = I*( div_erfc_b*(b+diff/s2)+div_erfc_a*(a-diff/s2)
              + b*b*e_b*s*erfc_b + a*a*e_a*s*erfc_a )/yErrors[i];
        out[i*nParam + 5] = 1/yErrors[i];

    }
}

} // namespace CurveFitting
} // namespace Mantid
