//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/Linear.h"
#include "MantidAPI/FunctionFactory.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

DECLARE_FUNCTION(Linear)

void Linear::init() {
  // default attributes and parameters
  // minimum number of points is 2 (point data, might be 3 for histograms)
  declareAttribute("n", Attribute(2));

  // non-fitting parameters
  declareAttribute("x0", Attribute(0.0));
  declareAttribute("x1", Attribute(1.0));

  declareParameter("y0", 0.0, "coefficient for constant term");
  declareParameter("y1", 0.0, "coefficient for linear term");
}

void Linear::function1D(double *out, const double *xValues,
                                  const size_t nData) const {
  const double a0 = getParameter("y0");
  const double a1 = getParameter("y1");
  const double x0 = getAttribute("x0").asDouble();
  const double x1 = getAttribute("x1").asDouble();

  const double slope = (a1 - a0) / (x1 - x0);
  const double constant_term = a1 - slope * x1;

  for (size_t i = 0; i < nData; i++) {
    out[i] = constant_term + slope * xValues[i];
  }

}

// for interpolation and not for fitting
void Linear::derivative1D(double *out, const double *xValues,
                                       const size_t nData,
                                       const size_t order) const {
  // silience unused warning
  (void)xValues;

  // throw error if the order is not the 1st or 2nd derivative
  if (order < 1)
    g_log.warning()
          << "Linear : order of derivative must be 1 or greater";

  if (order==1){
    const double a1 = getParameter("A1");
    std::fill_n(out, nData, a1);
  }
  else{
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

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
