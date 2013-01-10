//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/BackToBackExponential.h"
#include "MantidAPI/FunctionFactory.h"
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
    // Do not change the order of these parameters!
    declareParameter("I", 0.0);   // 0
    declareParameter("A", 1.0);   // 1
    declareParameter("B", 0.05);  // 2
    declareParameter("X0", 0.0);  // 3
    declareParameter("S", 1.0);   // 4
  }

  //------------------------------------------------------------------------------------------------
  double BackToBackExponential::height()const
  {
    double x0 = getParameter(3);
    std::vector<double> vec(1, x0);
    FunctionDomain1DVector domain(vec);
    FunctionValues values(domain);

    function(domain, values);

    return values[0];
  }


void BackToBackExponential::function1D(double* out, const double* xValues, const size_t nData)const
{
  /*
    const double& I = getParameter("I");
    const double& a = getParameter("A");
    const double& b = getParameter("B");
    const double& x0 = getParameter("X0");
    const double& s = getParameter("S");
  */

  const double& I = getParameter(0);
  const double& a = getParameter(1);
  const double& b = getParameter(2);
  const double& x0 = getParameter(3);
  const double& s = getParameter(4);

  double s2 = s*s;
  for (size_t i = 0; i < nData; i++) {
    double diff=xValues[i]-x0;
    if ( fabs(diff) < 100*s )
    {
      double val = 0.0;
      double arg1 = a/2*(a*s2+2*diff);
      if( arg1 < m_cutOff ) val += exp(arg1)*gsl_sf_erfc((a*s2+diff)/sqrt(2*s2)); //prevent overflow
      double arg2 = b/2*(b*s2-2*diff);
      if( arg2 < m_cutOff ) val += exp(arg2)*gsl_sf_erfc((b*s2-diff)/sqrt(2*s2)); //prevent overflow
      out[i] = I*val;
    }
    else
      out[i] = 0.0;
  }
}
void BackToBackExponential::functionDeriv1D(Jacobian* out, const double* xValues, const size_t nData)
{
  /*
    const double& I = getParameter("I");
    const double& a = getParameter("A");
    const double& b = getParameter("B");
    const double& x0 = getParameter("X0");
    const double& s = getParameter("S");
  */

  const double& I = getParameter(0);
  const double& a = getParameter(1);
  const double& b = getParameter(2);
  const double& x0 = getParameter(3);
  const double& s = getParameter(4);

  double s2 = s*s;
  for (size_t i = 0; i < nData; i++)
  {
    double diff = xValues[i]-x0;

    if ( fabs(diff) < 100*s )
    {

      double e_a = 0.0;
      double erfc_a = 0.0;
      double div_erfc_a = 0.0;
      double arg1 = 0.5*a*(a*s2+2*diff);
      if(arg1 < m_cutOff) //prevent overflow
      {
        e_a = exp(arg1);
        erfc_a = gsl_sf_erfc((a*s2+diff)/sqrt(2*s2));
        // apart from a prefactor terms arising from derivative or argument of erfc's divided by sqrt(2)
        div_erfc_a = - exp( -(a*s2+diff)*(a*s2+diff)/(2*s2)+0.5*a*(a*s2+2.0*diff) ) * M_SQRT2/M_SQRTPI;
      }

      double e_b = 0.0;
      double erfc_b = 0.0;
      double div_erfc_b = 0.0;
      double arg2 = 0.5*b*(b*s2-2*diff);
      if(arg2 < m_cutOff) //prevent overflow
      {
        e_b = exp(arg2);
        erfc_b = gsl_sf_erfc((b*s2-diff)/sqrt(2*s2));
        // apart from a prefactor terms arising from derivative or argument of erfc's divided by sqrt(2)
        div_erfc_b = - exp( -(b*s2-diff)*(b*s2-diff)/(2*s2)+0.5*b*(b*s2-2.0*diff) ) * M_SQRT2/M_SQRTPI;
      }

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
