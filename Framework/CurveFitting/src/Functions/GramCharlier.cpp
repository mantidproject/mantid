#include "MantidCurveFitting/Functions/GramCharlier.h"

namespace Mantid {
namespace CurveFitting {
namespace Functions {

/**
 * @brief GramCharlier::init Initialize the function parameters. This is
 * called by the base class at the appropriate time
 */
void GramCharlier::init() {
  // Default values are abitrary non-zero values that are "typical"
  // --------------------------- !!!WARNING!!! -------------------------
  // Do not reorder these parameters without altering the index access
  // in function1D
  // ----------------------------------------- -------------------------
  declareParameter("A", 0.01, "Area under the curve");
  declareParameter("X0", 0.2, "Position of the centroid");
  declareParameter("Sigma", 4, "Std. Deviation of distribution");
  declareParameter("C4", -0.005, "Coefficient of x^4 terms");
  declareParameter("C6", -0.003, "Coefficient of x^6 terms");
  declareParameter("C8", -0.002, "Coefficient of x^8 terms");
  declareParameter("C10", -0.001, "Coefficient of x^10 terms");
}

/**
 * @brief GramCharlier::function1D Computes the value of the function
 * @param out An array of size nData that will be filled with the computed
 * function values
 * @param xValues An array of size nData defining the input domain
 * @param nData The number of values in xData & out
 */
void GramCharlier::function1D(double *out, const double *xValues,
                              const size_t nData) const {
  // retrieve parameter values
  const double area(getParameter(0)), x0(getParameter(1)),
      sigma(getParameter(2)), c4(getParameter(3)), c6(getParameter(4)),
      c8(getParameter(5)), c10(getParameter(6));


}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
