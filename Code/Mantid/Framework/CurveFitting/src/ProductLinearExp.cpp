#include "MantidCurveFitting/ProductLinearExp.h"
#include "MantidCurveFitting/ExpDecay.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/ProductFunction.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid
{
  namespace CurveFitting
  {

    using namespace Kernel;
    using namespace API;

    DECLARE_FUNCTION(ProductLinearExp)

    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    ProductLinearExp::ProductLinearExp()
    {
      declareParameter("A0", 1.0);
      declareParameter("A1", 1.0);
      declareParameter("Height", 1.0);
      declareParameter("Lifetime", 1.0);
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ProductLinearExp::~ProductLinearExp()
    {
    }

    void ProductLinearExp::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
    {
      
    }

    void ProductLinearExp::function1D(double* out, const double* xValues, const size_t nData) const
    {
      double A0 = getParameter("A0");
      double A1 = getParameter("A1");
      double Height = getParameter("Height");
      double Lifetime = getParameter("Lifetime");

      for(size_t i = 0; i < nData; ++i)
      {
        out[i] = ((A1 * xValues[i]) + A0)* Height * std::exp(-xValues[i]/Lifetime); 
      }
    }

  } // namespace CurveFitting
} // namespace Mantid