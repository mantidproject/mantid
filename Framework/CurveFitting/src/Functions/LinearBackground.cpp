//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/LinearBackground.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(LinearBackground)

void LinearBackground::init() {
  declareParameter("A0", 0.0, "coefficient for constant term");
  declareParameter("A1", 0.0, "coefficient for linear term");
}

void LinearBackground::function1D(double *out, const double *xValues,
                                  const size_t nData) const {
  const double a0 = getParameter("A0");
  const double a1 = getParameter("A1");

  for (size_t i = 0; i < nData; i++) {
    out[i] = a0 + a1 * xValues[i];
  }
}

void LinearBackground::functionDeriv1D(Jacobian *out, const double *xValues,
                                       const size_t nData) {
  for (size_t i = 0; i < nData; i++) {
    out->set(i, 0, 1);
    out->set(i, 1, xValues[i]);
  }
}

/**
 * Do linear fit to the data in X and Y
 * @param X :: Vector with x-values
 * @param Y :: Vector with y-values
 */
void LinearBackground::fit(const std::vector<double> &X,
                           const std::vector<double> &Y) {
  if (X.size() != Y.size()) {
    throw std::runtime_error("Background fit: different array sizes");
  }
  size_t n = X.size();
  if (n == 0) {
    setParameter("A0", 0);
    setParameter("A1", 0);
    return;
  } else if (n == 1) {
    setParameter("A0", Y[0]);
    setParameter("A1", 0);
    return;
  }
  double x_mean = 0;
  double y_mean = 0;
  double x2_mean = 0;
  double xy_mean = 0;
  for (size_t i = 0; i < n; i++) {
    double x = X[i];
    double y = Y[i];
    x_mean += x;
    y_mean += y;
    x2_mean += x * x;
    xy_mean += x * y;
  }
  x_mean /= static_cast<double>(n);
  y_mean /= static_cast<double>(n);
  x2_mean /= static_cast<double>(n);
  xy_mean /= static_cast<double>(n);

  double a1 = (xy_mean - x_mean * y_mean) / (x2_mean - x_mean * x_mean);
  double a0 = y_mean - a1 * x_mean;

  setParameter("A0", a0);
  setParameter("A1", a1);
}

/// Calculate histogram data for the given bin boundaries.
/// @param out :: Output bin values (size == nBins) - integrals of the function
///    inside each bin.
/// @param left :: The left-most bin boundary.
/// @param right :: A pointer to an array of successive right bin boundaries
/// (size = nBins).
/// @param nBins :: Number of bins.
void LinearBackground::histogram1D(double *out, double left,
                                   const double *right,
                                   const size_t nBins) const {

  const double a0 = getParameter("A0");
  const double a1 = getParameter("A1");

  auto cumulFun = [a0, a1](double x) { return (a0 + 0.5 * a1 * x) * x; };

  double cLeft = cumulFun(left);
  for (size_t i = 0; i < nBins; ++i) {
    double cRight = cumulFun(right[i]);
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
void LinearBackground::histogramDerivative1D(Jacobian *jacobian, double left,
                                             const double *right,
                                             const size_t nBins) const {

  double xl = left;
  for (size_t i = 0; i < nBins; ++i) {
    double xr = right[i];
    jacobian->set(i, 0, xr - xl);
    jacobian->set(i, 1, 0.5 * (xr * xr - xl * xl));
    xl = xr;
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
