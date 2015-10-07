#include "MantidCurveFitting/FlatBackground.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidKernel/System.h"

using namespace Mantid::Kernel;
using namespace Mantid::API;

namespace Mantid {
namespace CurveFitting {
DECLARE_FUNCTION(FlatBackground)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
FlatBackground::FlatBackground() {}

//----------------------------------------------------------------------------------------------
/** Destructor
 */
FlatBackground::~FlatBackground() {}

/// Name of the function
std::string FlatBackground::name() const { return "FlatBackground"; }

/// The only parameter is the constant for the flat background.
void FlatBackground::init() {
  declareParameter("A0", 0.0, "coefficient for linear term");
}

/**
 * Evaluate the function at the supplied points.
 * @param out The y-values (output)
 * @param xValues The points to evaluate at
 * @param nData The number of points.
 */
void FlatBackground::function1D(double *out, const double *xValues,
                                const size_t nData) const {
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
void FlatBackground::functionDeriv1D(API::Jacobian *out, const double *xValues,
                                     const size_t nData) {
  UNUSED_ARG(xValues);

  for (size_t i = 0; i < nData; i++) {
    out->set(i, 0, 1);
  }
}

} // namespace Mantid
} // namespace CurveFitting
