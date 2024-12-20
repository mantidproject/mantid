// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidCurveFitting/Functions/FlatBackground.h"
#include "MantidAPI/FunctionFactory.h"

using namespace Mantid::Kernel;

using namespace Mantid::API;

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
DECLARE_FUNCTION(FlatBackground)

/// Name of the function
std::string FlatBackground::name() const { return "FlatBackground"; }

/// The only parameter is the constant for the flat background.
void FlatBackground::init() { declareParameter("A0", 0.0, "coefficient for linear term"); }

/**
 * Evaluate the function at the supplied points.
 * @param out The y-values (output)
 * @param xValues The points to evaluate at
 * @param nData The number of points.
 */
void FlatBackground::function1D(double *out, const double *xValues, const size_t nData) const {
  UNUSED_ARG(xValues);

  const double a0 = getParameter("A0");

  for (size_t i = 0; i < nData; i++) {
    out[i] = a0;
  }
}

/**
 * Evaluate the Jacobian at the supplied points.
 * @param out The Jacobian (output)
 * @param xValues The points to evaluate at
 * @param nData The number of points.
 */
void FlatBackground::functionDeriv1D(API::Jacobian *out, const double *xValues, const size_t nData) {
  UNUSED_ARG(xValues);

  for (size_t i = 0; i < nData; i++) {
    out->set(i, 0, 1);
  }
}

/// Calculate histogram data for the given bin boundaries.
/// @param out :: Output bin values (size == nBins) - integrals of the function
///    inside each bin.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void FlatBackground::histogram1D(double *out, double left, const double *right, const size_t nBins) const {

  const double a0 = getParameter("A0");

  double cLeft = a0 * left;
  for (size_t i = 0; i < nBins; ++i) {
    double cRight = a0 * right[i];
    out[i] = cRight - cLeft;
    cLeft = cRight;
  }
}

/// Derivatives of the histogram.
/// @param jacobian :: The output Jacobian.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void FlatBackground::histogramDerivative1D(Jacobian *jacobian, double left, const double *right,
                                           const size_t nBins) const {

  double xl = left;
  for (size_t i = 0; i < nBins; ++i) {
    double xr = right[i];
    jacobian->set(i, 0, xr - xl);
    xl = xr;
  }
}

} // namespace Mantid::CurveFitting::Functions
