//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/EndErfc.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>
#include <gsl/gsl_sf_erf.h>


namespace Mantid
{
  namespace CurveFitting
  {

    using namespace Kernel;
    using namespace API;

    DECLARE_FUNCTION(EndErfc)

    void EndErfc::init()
    {
      declareParameter("A", 2000.0); 
      declareParameter("B", 50.0); 
      declareParameter("C", 6.0); 
      declareParameter("D", 0.0); 
    }


    void EndErfc::function1D(double* out, const double* xValues, const size_t nData)const
    {
      const double& gA = getParameter("A"); 
      const double& gB = getParameter("B"); 
      const double& gC = getParameter("C"); 
      const double& gD = getParameter("D"); 


      for (size_t i = 0; i < nData; i++) {
        double x = xValues[i];
        out[i] = gA*gsl_sf_erfc((gB-x)/gC) + gD;
      }

      if( gA < 0) {
        for (size_t i = 0; i < nData; i++) {
          out[i] =- 2*gA;
        }
      }
    }

  } // namespace CurveFitting
} // namespace Mantid
