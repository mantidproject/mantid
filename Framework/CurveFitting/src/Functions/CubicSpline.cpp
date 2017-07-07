//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/CubicSpline.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Logger.h"
#include <iostream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
namespace {
/// static logger
Kernel::Logger g_log("CubicSpline");
}

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(CubicSpline)

/**Constructor
 *
 */
CubicSpline::CubicSpline()
    : m_min_points(3), m_acc(gsl_interp_accel_alloc(), m_gslFree),
      m_spline(gsl_spline_alloc(gsl_interp_cspline, m_min_points), m_gslFree) {
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
void CubicSpline::function1D(double *out, const double *xValues,
                             const size_t nData) const {
  // check if spline needs recalculating
  int n = getAttribute("n").asInt();

  boost::scoped_array<double> x(new double[n]);
  boost::scoped_array<double> y(new double[n]);

  // setup the reference points and calculate
  setupInput(x, y, n);

  calculateSpline(out, xValues, nData);
}

/** Sets up the spline object by with the parameters and attributes
 *
 * @param x :: The array of x values defining the spline
 * @param y :: The array of y values defining the spline
 * @param n :: The size of the arrays
 */
void CubicSpline::setupInput(boost::scoped_array<double> &x,
                             boost::scoped_array<double> &y, int n) const {
  // Populate data points from the input attributes and parameters
  bool xSortFlag = false;

  for (int i = 0; i < n; ++i) {
    std::string num = std::to_string(i);

    std::string xName = "x" + num;
    std::string yName = "y" + num;

    x[i] = getAttribute(xName).asDouble();

    if (!xSortFlag) {
      // if x[i] is out of order with its neighbours
      if (i > 1 && i < n && (x[i - 1] < x[i - 2] || x[i - 1] > x[i])) {
        xSortFlag = true;
      }
    }

    y[i] = getParameter(yName);
  }

  // sort the data points if necessary
  if (xSortFlag) {
    g_log.warning() << "Spline x parameters are not in ascending order. Values "
                       "will be sorted.\n";
    std::sort(x.get(), x.get() + n);
    std::sort(y.get(), y.get() + n);
  }

  // pass values to GSL objects
  initGSLObjects(x, y, n);
}

/** Calculate the derivatives for a set of points on the spline
 *
 * @param out :: The array to store the derivatives in
 * @param xValues :: The array of x values we wish to know the derivatives of
 * @param nData :: The size of the arrays
 * @param order :: The order of the derivatives o calculate
 */
void CubicSpline::derivative1D(double *out, const double *xValues, size_t nData,
                               const size_t order) const {
  int n = getAttribute("n").asInt();

  boost::scoped_array<double> x(new double[n]);
  boost::scoped_array<double> y(new double[n]);

  // setup the reference points and calculate
  setupInput(x, y, n);
  calculateDerivative(out, xValues, nData, order);
}

/** Check if the supplied x value falls within the range of the spline
 *
 * @param x :: The x value to check
 * @return Whether the value falls within the range of the spline
 */
bool CubicSpline::checkXInRange(double x) const {
  return (x >= m_spline->interp->xmin && x <= m_spline->interp->xmax);
}

/** Calculate the values on the spline at each point supplied
 *
 * @param out :: The array to store the calculated values
 * @param xValues :: The array of x values we wish to interpolate
 * @param nData :: The size of the arrays
 */
void CubicSpline::calculateSpline(double *out, const double *xValues,
                                  const size_t nData) const {
  int errorCode = 0;

  // calculate spline for given input set
  for (size_t i = 0; i < nData; ++i) {
    if (checkXInRange(xValues[i])) {
      out[i] = gsl_spline_eval(m_spline.get(), xValues[i], m_acc.get());
      errorCode =
          gsl_spline_eval_e(m_spline.get(), xValues[i], m_acc.get(), &out[i]);
    } else {
      if (xValues[i] < m_spline->interp->xmin) {
        out[i] = gsl_spline_eval(m_spline.get(), m_spline->interp->xmin,
                                 m_acc.get());
        errorCode = gsl_spline_eval_e(m_spline.get(), m_spline->interp->xmin,
                                      m_acc.get(), &out[i]);
      } else {
        out[i] = gsl_spline_eval(m_spline.get(), m_spline->interp->xmax,
                                 m_acc.get());
        errorCode = gsl_spline_eval_e(m_spline.get(), m_spline->interp->xmax,
                                      m_acc.get(), &out[i]);
      }
      g_log.warning()
          << "Some x values where out of range and will not be calculated.\n";
    }

    // check if GSL function returned an error
    checkGSLError(errorCode, GSL_EDOM);
  }
}

/** Calculate the derivatives of each of the supplied points
 *
 * @param out :: The array to store the calculated derivatives
 * @param xValues :: The array of x values we wish to calculate derivatives at
 * @param nData :: The size of the arrays
 * @param order :: The order of derivatives to calculate too
 */
void CubicSpline::calculateDerivative(double *out, const double *xValues,
                                      const size_t nData,
                                      const size_t order) const {
  double xDeriv = 0;
  int errorCode = 0;

  // throw error if the order is not the 1st or 2nd derivative
  if (order < 1)
    g_log.warning() << "CubicSpline: order of derivative must be 1 or greater";

  for (size_t i = 0; i < nData; ++i) {
    if (checkXInRange(xValues[i])) {
      // choose the order of the derivative
      if (order == 1) {
        xDeriv = gsl_spline_eval_deriv(m_spline.get(), xValues[i], m_acc.get());
        errorCode = gsl_spline_eval_deriv_e(m_spline.get(), xValues[i],
                                            m_acc.get(), &xDeriv);
      } else if (order == 2) {
        xDeriv =
            gsl_spline_eval_deriv2(m_spline.get(), xValues[i], m_acc.get());
        errorCode = gsl_spline_eval_deriv2_e(m_spline.get(), xValues[i],
                                             m_acc.get(), &xDeriv);
      }
    } else {
      xDeriv = 0;
      g_log.warning()
          << "Some x values where out of range and will not be calculated.\n";
    }

    // check GSL functions didn't return an error
    checkGSLError(errorCode, GSL_EDOM);

    // record the value
    out[i] = xDeriv;
  }
}

/** Set an attribute for the function
 *
 * @param attName :: The name of the attribute to set
 * @param att :: The attribute to set
 */
void CubicSpline::setAttribute(const std::string &attName,
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

      // reallocate gsl object to new size
      reallocGSLObjects(n);

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
          "Cubic Spline: Can't decrease the number of attributes");
    }
  }

  storeAttributeValue(attName, att);
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

    std::string message("CubicSpline: ");
    message.append(gsl_strerror(errorType));

    throw std::runtime_error(message);
  }
}

/** Initilize the GSL spline with the given points
 *
 * @param x :: The x points defining the spline
 * @param y :: The y points defining the spline
 * @param n :: The size of the arrays
 */
void CubicSpline::initGSLObjects(boost::scoped_array<double> &x,
                                 boost::scoped_array<double> &y, int n) const {
  int status = gsl_spline_init(m_spline.get(), x.get(), y.get(), n);
  checkGSLError(status, GSL_EINVAL);
}

/** Reallocate the size of the GSL objects
 *
 * @param n :: The new size of the spline object
 */
void CubicSpline::reallocGSLObjects(const int n) {
  m_spline.reset(gsl_spline_alloc(gsl_interp_cspline, n), m_gslFree);
  gsl_interp_accel_reset(m_acc.get());
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
