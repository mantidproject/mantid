#include "MantidCurveFitting/Functions/GramCharlier.h"

#include "MantidAPI/FunctionFactory.h"

#include <boost/math/special_functions/hermite.hpp>

namespace Mantid {
namespace CurveFitting {
namespace Functions {

DECLARE_FUNCTION(GramCharlier)

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
  declareParameter("A", 0.01, "Amplitude");
  declareParameter("X0", 0.2, "Position of the centroid");
  declareParameter("Sigma", 4, "Std. Deviation of distribution");
  declareParameter("C4", -0.005, "Coefficient of 4th Hermite polynomial");
  declareParameter("C6", -0.003, "Coefficient of 6th Hermite polynomial");
  declareParameter("C8", -0.002, "Coefficient of 8th Hermite polynomial");
  declareParameter("C10", -0.001, "Coefficient of 10th Hermite polynomial");
  declareParameter("Afse", 0.01, "Ampliude of final-state effects term");
}

/**
 * @brief GramCharlier::function1D Computes the value of the function
 * @param out An array of size nData that will be filled with the computed
 * function values
 * @param x An array of size nData defining the input domain
 * @param n The number of values in in & out
 */
void GramCharlier::function1D(double *out, const double *x,
                              const size_t n) const {
  using boost::math::hermite;
  using std::exp;
  using std::sqrt;
  using std::pow;
  // formula
  // f(x) = Aexp(-z^2)*(1/sqrt(2*Pi*Sigma^2))*(1 + (C4/2^4/(4/2)!)*H_4(z) +
  //        (C6/2^6/(6/2)!)*H_6(z) + (C8/2^8/(8/2)!)*H_8(z) +
  //        (C10/2^10/(10/2)!)*H_10(z)) +
  //        (Afse*Sigma*sqrt(2)/12)*(1/sqrt(2*Pi*Sigma^2))*exp(-z^2)*H_3(z))
  //
  // where z=((x-X0)/Sigma/sqrt(2))

  // retrieve parameter values and scale coefficients once
  const double amp(getParameter(0)), x0(getParameter(1)),
      sigma(getParameter(2)), c4(getParameter(3) / 32.0),
      c6(getParameter(4) / 384.0), c8(getParameter(5) / 6144.0),
      c10(getParameter(6) / 122880.0), ampFSE(getParameter(7));
  const double root2Sigma = sqrt(2) * sigma;
  const double norm = 1 / (root2Sigma * sqrt(M_PI));
  const double prefactorFSE = root2Sigma / 12.0;

  for (size_t i = 0; i < n; ++i) {
    const double z = (x[i] - x0) / root2Sigma;
    out[i] = amp * norm * exp(-z * z) *
                 (1 + c4 * hermite(4, z) + c6 * hermite(6, z) +
                  c8 * hermite(8, z) + c10 * hermite(10, z)) +
             ampFSE * norm * prefactorFSE * exp(-z * z) * hermite(3, z);
  }
}

} // namespace Functions
} // namespace CurveFitting
} // namespace Mantid
