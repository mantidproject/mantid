//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BackToBackExponential.h"
#include <gsl/gsl_sf_erf.h>
#include <gsl/gsl_multifit_nlin.h>

namespace Mantid
{
namespace CurveFitting
{

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(BackToBackExponential)

void BackToBackExponential::init()
{
  declareParameter("I", 0.0);
  declareParameter("A", 1.0);
  declareParameter("B", 0.05);
  declareParameter("X0", 0.0);
  declareParameter("S", 1.0);
}


void BackToBackExponential::function(double* out, const double* xValues, const int& nData)const
{
    const double& I = getParameter("I");
    const double& a = getParameter("A");
    const double& b = getParameter("B");
    const double& x0 = getParameter("X0");
    const double& s = getParameter("S");

    double s2 = s*s;
    for (int i = 0; i < nData; i++) {
      double diff=xValues[i]-x0;
      if ( fabs(diff) < 10*s )
      {
        out[i] = I*(exp(a/2*(a*s2+2*diff))*gsl_sf_erfc((a*s2+diff)/sqrt(2*s2))
                    + exp(b/2*(b*s2-2*diff))*gsl_sf_erfc((b*s2-diff)/sqrt(2*s2)));
      }
      else
        out[i] = 0.0;
    }
}

void BackToBackExponential::functionDeriv(Jacobian* out, const double* xValues, const int& nData)
{
    const double& I = getParameter("I");
    const double& a = getParameter("A");
    const double& b = getParameter("B");
    const double& x0 = getParameter("X0");
    const double& s = getParameter("S");

    double s2 = s*s;
    for (int i = 0; i < nData; i++) {
      double diff = xValues[i]-x0;

      if ( fabs(diff) < 10*s )
      {

        double e_a = exp(0.5*a*(a*s2+2*diff));
        double e_b = exp(0.5*b*(b*s2-2*diff));
        double erfc_a = gsl_sf_erfc((a*s2+diff)/sqrt(2*s2));
        double erfc_b = gsl_sf_erfc((b*s2-diff)/sqrt(2*s2));

        // apart from a prefactor terms arising from defivative or argument of erfc's divided by sqrt(2)
        double div_erfc_a = - exp( -(a*s2+diff)*(a*s2+diff)/(2*s2)+0.5*a*(a*s2+2.0*diff) ) * M_SQRT2/M_SQRTPI;
        double div_erfc_b = - exp( -(b*s2-diff)*(b*s2-diff)/(2*s2)+0.5*b*(b*s2-2.0*diff) ) * M_SQRT2/M_SQRTPI;

        out->set(i,0, (e_a*erfc_a+e_b*erfc_b));
        out->set(i,1, I*( s*div_erfc_a + e_a*(a*s2+diff)*erfc_a ));
        out->set(i,2, I*( s*div_erfc_b + e_b*(b*s2-diff)*erfc_b ));
        out->set(i,3, I*( (-div_erfc_a+div_erfc_b)/s + b*e_b*erfc_b - a*e_a*erfc_a ));
        out->set(i,4, I*( div_erfc_b*(b+diff/s2)+div_erfc_a*(a-diff/s2)
              + b*b*e_b*s*erfc_b + a*a*e_a*s*erfc_a ));
      }
      else
      {
        out->set(i,0, 0.0);
        out->set(i,1, 0.0);
        out->set(i,2, 0.0);
        out->set(i,3, 0.0);
        out->set(i,4, 0.0);
      }
    }
}


} // namespace CurveFitting
} // namespace Mantid
