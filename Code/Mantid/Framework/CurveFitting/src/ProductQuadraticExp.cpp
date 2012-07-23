#include "MantidCurveFitting/ProductQuadraticExp.h"

namespace Mantid
{
  namespace CurveFitting
  {
    //----------------------------------------------------------------------------------------------
    /** Constructor
    */
    ProductQuadraticExp::ProductQuadraticExp()
    {
      declareParameter("A0", 0.0);
      declareParameter("A1", 0.0);
      declareParameter("A2", 0.0);
      declareParameter("Height", 1.0);
      declareParameter("Lifetime", 1.0);
    }

    //----------------------------------------------------------------------------------------------
    /** Destructor
    */
    ProductQuadraticExp::~ProductQuadraticExp()
    {
    }

    void ProductQuadraticExp::functionDeriv1D(API::Jacobian* out, const double* xValues, const size_t nData)
    {
      throw std::runtime_error("Not Implemented");
    }

    void ProductQuadraticExp::function1D(double* out, const double* xValues, const size_t nData) const
    {
      double A0 = getParameter("A0");
      double A1 = getParameter("A1");
      double A2 = getParameter("A2");
      double Height = getParameter("Height");
      double Lifetime = getParameter("Lifetime");

      UNUSED_ARG(A0);
      UNUSED_ARG(A1);
      UNUSED_ARG(A2);
      UNUSED_ARG(Height);
      UNUSED_ARG(Lifetime);

      throw std::runtime_error("Not Implemented");
    }

  } // namespace CurveFitting
} // namespace Mantid