// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/CubicSpline.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Logger.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <vector>

#include <gsl/gsl_errno.h>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;
namespace {
/// static logger
Kernel::Logger g_log("CubicSpline");
} // namespace

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(CubicSpline)

/**Constructor
 *
 */
CubicSpline::CubicSpline()
    : m_min_points(3), m_acc(Kernel::spline::make_interp_accel()),
      m_spline(gsl_spline_alloc(gsl_interp_cspline, m_min_points)), m_recalculateSpline(true) {
  // setup class with a default set of attributes
  declareAttribute("n", Attribute(m_min_points));

  // declare corresponding attributes and parameters
  declareAttribute("x0", Attribute(0.0));
  declareAttribute("x1", Attribute(1.0));
  declareAttribute("x2", Attribute(2.0));

  declareParameter("y0", 0);
  declareParameter("y1", 0);
  declareParameter("y2", 0);
}

/** Execute the function
 *
 * @param out :: The array to store the calculated y values
 * @param xValues :: The array of x values to interpolate
 * @param nData :: The size of the arrays
 */
void CubicSpline::function1D(double *out, const double *xValues, const size_t nData) const {
  // check if spline needs recalculating
  int n = getAttribute("n").asInt();

  // setup the reference points and calculate
  if (m_recalculateSpline)
    setupInput(n);

  calculateSpline(out, xValues, nData);
}

/** Sets up the spline object by with the parameters and attributes
 *
 * @param n :: The size of the arrays
 */
void CubicSpline::setupInput(int const n) const {
  // Populate data points from the input attributes and parameters
  bool isSorted = true;

  std::vector<double> x(n);
  std::vector<double> y(n);

  for (int i = 0; i < n; ++i) {

    x[i] = getAttribute("x" + std::to_string(i)).asDouble();
    y[i] = getParameter(i);

    if (isSorted) {
      // if x[i] is out of order with its neighbours
      if (i > 1 && i < n && (x[i - 1] < x[i - 2] || x[i - 1] > x[i])) {
        isSorted = false;
      }
    }
  }

  // sort the data points if necessary
  if (!isSorted) {
    g_log.warning() << "Spline x parameters are not in ascending order. Values "
                       "will be sorted.\n";

    using point = std::pair<double, double>;
    std::vector<point> pairs;
    pairs.reserve(n);
    for (int i = 0; i < n; ++i) {
      pairs.emplace_back(x[i], y[i]);
    }

    std::sort(pairs.begin(), pairs.end(), [](const point &xy1, const point &xy2) { return xy1.first < xy2.first; });

    for (int i = 0; i < n; ++i) {
      x[i] = pairs[i].first;
      y[i] = pairs[i].second;
    }
  }

  // initialize the spline
  int status = gsl_spline_init(m_spline.get(), x.data(), y.data(), n);
  checkGSLError(status, GSL_EINVAL);
  m_recalculateSpline = false;
}

/** Calculate the derivatives for a set of points on the spline
 *
 * @param out :: The array to store the derivatives in
 * @param xValues :: The array of x values we wish to know the derivatives of
 * @param nData :: The size of the arrays
 * @param order :: The order of the derivatives o calculate
 */
void CubicSpline::derivative1D(double *out, const double *xValues, size_t nData, const size_t order) const {
  int n = getAttribute("n").asInt();

  // setup the reference points and calculate
  if (m_recalculateSpline)
    setupInput(n);

  calculateDerivative(out, xValues, nData, order);
}

/** Check if the supplied x value falls within the range of the spline
 *
 * @param x :: The x value to check
 * @return Whether the value falls within the range of the spline
 */
bool CubicSpline::checkXInRange(double x) const { return (x >= m_spline->interp->xmin && x <= m_spline->interp->xmax); }

/** Calculate the values on the spline at each point supplied
 *
 * @param out :: The array to store the calculated values
 * @param xValues :: The array of x values we wish to interpolate
 * @param nData :: The size of the arrays
 */
void CubicSpline::calculateSpline(double *out, const double *xValues, const size_t nData) const {
  // calculate spline for given input set
  bool outOfRange(false);
  for (size_t i = 0; i < nData; ++i) {
    if (checkXInRange(xValues[i])) {
      // calculate the y value
      out[i] = splineEval(xValues[i]);
    } else {
      // if out of range, set it to constant of fringe values
      outOfRange = true;
      if (xValues[i] < m_spline->interp->xmin) {
        out[i] = splineEval(m_spline->interp->xmin);
      } else {
        out[i] = splineEval(m_spline->interp->xmax);
      }
    }
  }

  // inform user that some values weren't calculated
  if (outOfRange) {
    g_log.information() << "Some x values where out of range and will not be calculated.\n";
  }
}

/**Evaluate a point on the spline. Includes basic error handling
 *
 * @param x :: Point to evaluate
 * @return :: the value of the spline at the given point
 */
double CubicSpline::splineEval(const double x) const {
  // calculate the y value
  double y;
  int errorCode = gsl_spline_eval_e(m_spline.get(), x, m_acc.get(), &y);

  // check if GSL function returned an error
  checkGSLError(errorCode, GSL_EDOM);

  return y;
}

/** Calculate the derivatives of each of the supplied points
 *
 * @param out :: The array to store the calculated derivatives
 * @param xValues :: The array of x values we wish to calculate derivatives at
 * @param nData :: The size of the arrays
 * @param order :: The order of derivatives to calculate too
 */
void CubicSpline::calculateDerivative(double *out, const double *xValues, const size_t nData,
                                      const size_t order) const {
  double xDeriv = 0;
  int errorCode = 0;
  bool outOfRange(false);

  // throw error if the order is not the 1st or 2nd derivative
  if (order < 1)
    throw std::invalid_argument("CubicSpline: order of derivative must be 1 or greater");

  for (size_t i = 0; i < nData; ++i) {
    if (checkXInRange(xValues[i])) {
      // choose the order of the derivative
      if (order == 1) {
        errorCode = gsl_spline_eval_deriv_e(m_spline.get(), xValues[i], m_acc.get(), &xDeriv);
      } else if (order == 2) {
        errorCode = gsl_spline_eval_deriv2_e(m_spline.get(), xValues[i], m_acc.get(), &xDeriv);
      }
    } else {
      // if out of range, just set it to zero
      outOfRange = true;
      xDeriv = 0;
    }

    // check GSL functions didn't return an error
    checkGSLError(errorCode, GSL_EDOM);

    // record the value
    out[i] = xDeriv;
  }

  // inform user that some values weren't calculated
  if (outOfRange) {
    g_log.information() << "Some x values were out of range and will not be calculated.";
  }
}

/** Set a parameter for the function and flags the spline for re-calculation
 *
 * @param i :: index of parameter
 * @param value :: value of parameter
 * @param explicitlySet :: whether it's value was explicitly set or not
 */
void CubicSpline::setParameter(size_t i, const double &value, bool explicitlySet) {
  // Call parent setParameter implementation
  ParamFunction::setParameter(i, value, explicitlySet);

  // recalculate if necessary
  m_recalculateSpline = true;
}

/** Set an attribute for the function
 *
 * @param attName :: The name of the attribute to set
 * @param att :: The attribute to set
 */
void CubicSpline::setAttribute(const std::string &attName, const API::IFunction::Attribute &att) {

  if (attName == "n") {
    // get the new and old number of data points
    int n = att.asInt();
    int oldN = getAttribute("n").asInt();

    // check that the number of data points is in a valid range
    if (n > oldN) {
      // get the name of the last x data point
      std::string oldXName = "x" + std::to_string(oldN - 1);
      double oldX = getAttribute(oldXName).asDouble();

      // reallocate gsl object to new size
      reallocGSLObjects(n);

      // create blank a number of new blank parameters and attributes
      for (int i = oldN; i < n; ++i) {
        std::string num = std::to_string(i);

        std::string newXName = "x" + num;
        std::string newYName = "y" + num;

        declareAttribute(newXName, Attribute(oldX + static_cast<double>(i - oldN + 1)));
        declareParameter(newYName, 0);
      }

      // flag that the spline + derivatives will now need to be recalculated
      m_recalculateSpline = true;
    } else if (n < oldN) {
      throw std::invalid_argument("Cubic Spline: Can't decrease the number of attributes");
    }
  }

  storeAttributeValue(attName, att);
}

/** Set an x attribute for the spline
 *
 * @param index :: index of x attribute to set
 * @param x :: The value of the x attribute
 */
void CubicSpline::setXAttribute(const size_t index, double x) {
  size_t n = static_cast<size_t>(getAttribute("n").asInt());

  // check that setting the x attribute is within our range
  if (index < n) {
    std::string xName = "x" + std::to_string(index);
    setAttributeValue(xName, x);

    // attributes updated, flag for recalculation
    m_recalculateSpline = true;
  } else {
    throw std::range_error("Cubic Spline: x index out of range.");
  }
}

/** Checks if a call to a GSL function produced a given error or not
 * and throw an appropriate message
 *
 * @param status :: The status returned for the GSL function call
 * @param errorType :: The type of GSL error to check for
 */
void CubicSpline::checkGSLError(const int status, const int errorType) const {
  // check GSL functions didn't return an error
  if (status == errorType) {
    m_recalculateSpline = true;

    std::string message("CubicSpline: ");
    message.append(gsl_strerror(errorType));

    throw std::runtime_error(message);
  }
}

/** Reallocate the size of the GSL objects
 *
 * @param n :: The new size of the spline object
 */
void CubicSpline::reallocGSLObjects(const int n) {
  m_spline.reset(gsl_spline_alloc(gsl_interp_cspline, n));
  gsl_interp_accel_reset(m_acc.get());
}

} // namespace Mantid::CurveFitting::Functions
