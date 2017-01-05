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
  // default parameters
  declareParameter("A0", 0.0, "coefficient for constant term");
  declareParameter("A1", 0.0, "coefficient for linear term");

  // default attributes
  // minimum number of points is 2 (point data)
  declareAttribute("n", Attribute(2));

  declareAttribute("y0", Attribute(0.0));
  declareAttribute("y1", Attribute(0.0));
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

void LinearBackground::function1DEval(double *out, const double *xValues,
                        const size_t nData) const {
  const double y1 = getAttribute("y1").asDouble();
  const double x1 = getAttribute("x1").asDouble();

  const double constant_term = y1 - getSlope() * x1;

  for (size_t i = 0; i < nData; i++) {
    out[i] = constant_term + getSlope() * xValues[i];
  }
}

// for interpolation and not for fitting
void LinearBackground::derivative1DEval(double *out, const double *xValues,
                          const size_t nData, const size_t order) const {
  // silience unused warning
  (void)xValues;

  // throw error if the order is not the 1st or 2nd derivative
  if (order < 1)
    g_log.warning() << "Linear : order of derivative must be 1 or greater";

  if (order == 1) {
    std::fill_n(out, nData, getSlope());
  } else {
    std::fill_n(out, nData, 0.0);
  }
}

/** Set an attribute for the function
 *
 * @param attName :: The name of the attribute to set
 * @param att :: The attribute to set
 */
void LinearBackground::setAttribute(const std::string &attName,
                          const API::IFunction::Attribute &att) {

  if (attName == "n") {
    // get the new and old number of data points
    int n = att.asInt();
    int oldN = getAttribute("n").asInt();

    // check that the number of data points is in a valid range
    if (n > oldN) {
      // get the name of the last x data point
      std::string oldXName = "x" + std::to_string(oldN - 1);
      double oldX = getAttribute(oldXName).asDouble();

      // create blank a number of new blank parameters and attributes
      for (int i = oldN; i < n; ++i) {
        std::string num = std::to_string(i);

        std::string newXName = "x" + num;
        std::string newYName = "y" + num;

        declareAttribute(newXName,
                         Attribute(oldX + static_cast<double>(i - oldN + 1)));
        declareAttribute(newYName, Attribute(0.0));
      }
    } else if (n < oldN) {
      throw std::invalid_argument(
          "Linear : Can't decrease the number of attributes");
    }
  }

  storeAttributeValue(attName, att);
}

/** Get slope of the linear function (only first two points needed)
 *
 * @return slope :: The slope of the linear function
 */
double LinearBackground::getSlope() const {
  const double y0 = getAttribute("y0").asDouble();
  const double y1 = getAttribute("y1").asDouble();
  const double x0 = getAttribute("x0").asDouble();
  const double x1 = getAttribute("x1").asDouble();

  return (y1 - y0) / (x1 - x0);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
