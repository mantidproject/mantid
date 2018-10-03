// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/ProductQuadraticExp.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/Functions/ExpDecay.h"
#include "MantidCurveFitting/Functions/ProductFunction.h"
#include "MantidCurveFitting/Functions/Quadratic.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

DECLARE_FUNCTION(ProductQuadraticExp)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ProductQuadraticExp::ProductQuadraticExp() {
  declareParameter("A0", 0.0, "Coefficient for constant term");
  declareParameter("A1", 0.0, "Coefficient for linear term");
  declareParameter("A2", 0.0, "Coefficient for quadratic term");
  declareParameter("Height", 1.0, "Height");
  declareParameter("Lifetime", 1.0, "Lifetime");
}

/**
Calculate the 1D function derivatives.
@param out : Jacobian to set derivates on.
@param xValues : Domain x-values.
@param nData : Number of elements.
*/
void ProductQuadraticExp::functionDeriv1D(API::Jacobian *out,
                                          const double *xValues,
                                          const size_t nData) {
  const double A0 = getParameter("A0");
  const double A1 = getParameter("A1");
  const double A2 = getParameter("A2");
  const double Height = getParameter("Height");
  const double Lifetime = getParameter("Lifetime");

  for (size_t i = 0; i < nData; i++) {
    double x = xValues[i];
    double expComponent = Height * std::exp(-x / Lifetime);
    double linearComponent = (A1 * x) + A0;

    out->set(i, 0, ((A2 * x * x) + (A1 * x)) * expComponent);
    out->set(i, 1, ((A2 * x * x) + x + A0) * expComponent);
    out->set(i, 2, ((x * x) + (A1 * x) + A0) * expComponent);
    out->set(i, 3, linearComponent * expComponent / Height);
    out->set(i, 4, linearComponent * expComponent * x / (Lifetime * Lifetime));
  }
}

/**
Evaluate the 1D function
@param out : Out values.
@param xValues : Domain x-values.
@param nData : Number of elements.
*/
void ProductQuadraticExp::function1D(double *out, const double *xValues,
                                     const size_t nData) const {
  const double A0 = getParameter("A0");
  const double A1 = getParameter("A1");
  const double A2 = getParameter("A2");
  const double Height = getParameter("Height");
  const double Lifetime = getParameter("Lifetime");

  for (size_t i = 0; i < nData; ++i) {
    out[i] = (A0 + (A1 * xValues[i]) + (A2 * xValues[i] * xValues[i])) *
             Height * std::exp(-xValues[i] / Lifetime);
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
