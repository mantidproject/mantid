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
  // default attributes and parameters
  // minimum number of points is 2 (point data, might be 3 for histograms)
  declareAttribute("n", Attribute(2));

  // non-fitting parameters
  declareAttribute("x0", Attribute(0.0));
  declareAttribute("x1", Attribute(1.0));

  declareParameter("A0", 0.0, "coefficient for constant term");
  declareParameter("A1", 0.0, "coefficient for linear term");
}

void LinearBackground::function1D(double *out, const double *xValues,
                                  const size_t nData) const {
  const double a0 = getParameter("A0");
  const double a1 = getParameter("A1");
  const double x0 = getAttribute("x0").asDouble();
  const double x1 = getAttribute("x1").asDouble();

  const double slope = (a1 - a0) / (x1 - x0);
  const double constant_term = a1 - slope * x1;

  for (size_t i = 0; i < nData; i++) {
    out[i] = constant_term + slope * xValues[i];
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
        declareParameter(newYName, 0);
      }
    } else if (n < oldN) {
      throw std::invalid_argument(
          "Linear Background: Can't decrease the number of attributes");
    }
  }

  storeAttributeValue(attName, att);
}

void LinearBackground::derivative1D(double *out, const double *xValues,
                                       const size_t nData,
                                       const size_t order) const {
  // silience unused warning
  (void)xValues;

  // throw error if the order is not the 1st or 2nd derivative
  if (order < 1)
    g_log.warning()
          << "FlatBackground: order of derivative must be 1 or greater";

  if (order==1){
    const double a1 = getParameter("A1");
    std::fill_n(out, nData, a1);
  }
  else{
    std::fill_n(out, nData, 0.0);
  }
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
