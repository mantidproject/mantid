//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Linear.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/Logger.h"
#include <iostream>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;
namespace {
/// static logger
Kernel::Logger g_log("Linear");
}

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(Linear)

void Linear::init() {
  // default attributes and parameters
  // minimum number of points is 2 (point data)
  declareAttribute("n", Attribute(2));

  // non-fitting parameters
  declareAttribute("x0", Attribute(0.0));
  declareAttribute("x1", Attribute(1.0));

  declareParameter("y0", 0.0);
  declareParameter("y1", 0.0);
}

void Linear::function1D(double *out, const double *xValues,
                        const size_t nData) const {
  const double y1 = getParameter("y1");
  const double x1 = getAttribute("x1").asDouble();

  const double constant_term = y1 - getSlope() * x1;

  for (size_t i = 0; i < nData; i++) {
    out[i] = constant_term + getSlope() * xValues[i];
  }
}

// for interpolation and not for fitting
void Linear::derivative1D(double *out, const double *xValues,
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
void Linear::setAttribute(const std::string &attName,
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
          "Linear : Can't decrease the number of attributes");
    }
  }

  storeAttributeValue(attName, att);
}

/** Get slope of the linear function (only first two points needed)
 *
 * @return slope :: The slope of the linear function
 */
double Linear::getSlope() const {
  const double y0 = getParameter("y0");
  const double y1 = getParameter("y1");
  const double x0 = getAttribute("x0").asDouble();
  const double x1 = getAttribute("x1").asDouble();

  return (y1 - y0) / (x1 - x0);
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
