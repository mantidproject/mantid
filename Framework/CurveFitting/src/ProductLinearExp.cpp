#include "MantidCurveFitting/ProductLinearExp.h"
#include "MantidCurveFitting/ExpDecay.h"
#include "MantidCurveFitting/LinearBackground.h"
#include "MantidCurveFitting/ProductFunction.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(ProductLinearExp)

//----------------------------------------------------------------------------------------------
/** Constructor
*/
ProductLinearExp::ProductLinearExp() {
  declareParameter("A0", 1.0);
  declareParameter("A1", 1.0);
  declareParameter("Height", 1.0);
  declareParameter("Lifetime", 1.0);
}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
ProductLinearExp::~ProductLinearExp() {}

/**
Calculate the 1D function derivatives.
@param out : Jacobian to set derivates on.
@param xValues : Domain x-values.
@param nData : Number of elements.
*/
void ProductLinearExp::functionDeriv1D(API::Jacobian *out,
                                       const double *xValues,
                                       const size_t nData) {
  const double A0 = getParameter("A0");
  const double A1 = getParameter("A1");
  const double Height = getParameter("Height");
  const double Lifetime = getParameter("Lifetime");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double expComponent = Height * std::exp(-x / Lifetime);
    double linearComponent = (A1 * x) + A0;

    out->set(i, 0, A1 * x * expComponent);
    out->set(i, 1, (x + A0) * expComponent);
    out->set(i, 2, linearComponent * expComponent / Height);
    out->set(i, 3, linearComponent * expComponent * x / (Lifetime * Lifetime));
  }
}

/**
Evaluate the 1D function
@param out : Out values.
@param xValues : Domain x-values.
@param nData : Number of elements.
*/
void ProductLinearExp::function1D(double *out, const double *xValues,
                                  const size_t nData) const {
  const double A0 = getParameter("A0");
  const double A1 = getParameter("A1");
  const double Height = getParameter("Height");
  const double Lifetime = getParameter("Lifetime");

  for (size_t i = 0; i < nData; ++i) {
    out[i] =
        ((A1 * xValues[i]) + A0) * Height * std::exp(-xValues[i] / Lifetime);
  }
}

} // namespace CurveFitting
} // namespace Mantid
