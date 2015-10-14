//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "LorentzianTest.h"
#include "MantidAPI/Jacobian.h"
#include "MantidAPI/FunctionFactory.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {

using namespace Kernel;
using namespace API;

DECLARE_FUNCTION(LorentzianTest)

// ** MODIFY THIS **
// Here specify/declare the parameters of your Fit Function
//
// declareParameter takes three arguments:
//
//   1st: The name of the parameter
//   2nd: The default (initial) value of the parameter
//   3rd: A description of the parameter (optional)
//
void LorentzianTest::init() {
  declareParameter("Height", 0.0, "Height at peak maximum");
  declareParameter("PeakCentre", 0.0, "Centre of peak");
  declareParameter("HWHM", 0.0, "Half-Width Half-Maximum");
}

// ** MODIFY THIS **
// So this is where you specify your function!
// It arguments have the following meaning:
//
//     xValues - is an array of size nData which contains the
//               x-values for which you want to calculate the
//               function
//     out     - is likewise an array of size nData and must
//               output the function values at the x-values
//               provided in the xValues array
//     nData   - the number of data points where a function value
//               must be calculated
void LorentzianTest::functionLocal(double *out, const double *xValues,
                                   const size_t nData) const {
  const double &height = getParameter("Height");
  const double &peakCentre = getParameter("PeakCentre");
  const double &hwhm = getParameter("HWHM");

  // This for-loop calculates the function for nData data-points
  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - peakCentre;
    out[i] = height * (hwhm * hwhm / (diff * diff + hwhm * hwhm));
  }
}

// ** MODIFY THIS **
// If you know how to calculate the derivative of your fitting function - here
// is your chance!
// If not then instead use the following code:
//      void LorentzianTest::functionDerivLocal(API::Jacobian* out, const
//      double* xValues, const int& nData)
//      {
//        calNumericalDeriv(out, xValues, nData);
//      }
// I.e. substitute the code below with the four lines of code above
void LorentzianTest::functionDerivLocal(Jacobian *out, const double *xValues,
                                        const size_t nData) {
  const double &height = getParameter("Height");
  const double &peakCentre = getParameter("PeakCentre");
  const double &hwhm = getParameter("HWHM");

  for (size_t i = 0; i < nData; i++) {
    double diff = xValues[i] - peakCentre;
    double invDenominator = 1 / ((diff * diff + hwhm * hwhm));
    out->set(i, 0, hwhm * hwhm * invDenominator);
    out->set(i, 1, 2.0 * height * diff * hwhm * hwhm * invDenominator *
                       invDenominator);
    out->set(i, 2, height * (-hwhm * hwhm * invDenominator + 1) * 2.0 * hwhm *
                       invDenominator);
  }
}

} // namespace CurveFitting
} // namespace Mantid
