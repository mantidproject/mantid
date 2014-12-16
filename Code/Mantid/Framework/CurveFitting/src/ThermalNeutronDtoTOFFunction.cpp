#include "MantidCurveFitting/ThermalNeutronDtoTOFFunction.h"
#include "MantidKernel/System.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidAPI/FunctionDomain1D.h"
#include <gsl/gsl_sf_erf.h>
#include <cmath>

#define PI 3.14159265358979323846264338327950288419716939937510

using namespace Mantid::API;
using namespace std;

namespace Mantid {
namespace CurveFitting {

//----------------------------------------------------------------------------------------------
DECLARE_FUNCTION(ThermalNeutronDtoTOFFunction)

//----------------------------------------------------------------------------------------------
/** Constructor
 */
ThermalNeutronDtoTOFFunction::ThermalNeutronDtoTOFFunction() {}

//----------------------------------------------------------------------------------------------
/** Destructor
*/
ThermalNeutronDtoTOFFunction::~ThermalNeutronDtoTOFFunction() {}

/**
* Define the fittable parameters
*/
void ThermalNeutronDtoTOFFunction::init() {

  /// Instrument geometry related
  declareParameter("Dtt1", 1.0);  // 0
  declareParameter("Dtt1t", 1.0); // 1
  declareParameter("Dtt2t", 1.0); // 2
  declareParameter("Zero", 0.0);  // 3
  declareParameter("Zerot", 0.0); // 4

  declareParameter("Width", 1.0);  // 5
  declareParameter("Tcross", 1.0); // 6
}

/** Main function
  * xValues containing the d-space value of peaks centres
  */
void ThermalNeutronDtoTOFFunction::function1D(double *out,
                                              const double *xValues,
                                              const size_t nData) const {
  double dtt1 = getParameter("Dtt1");
  double dtt1t = getParameter("Dtt1t");
  double dtt2t = getParameter("Dtt2t");
  double zero = getParameter("Zero");
  double zerot = getParameter("Zerot");
  double width = getParameter("Width");
  double tcross = getParameter("Tcross");

  for (size_t i = 0; i < nData; ++i) {
    // out[i] = corefunction(xValues[i], dtt1, dtt1t, dtt2t, zero, zerot, width,
    // tcross);
    out[i] = calThermalNeutronTOF(xValues[i], dtt1, dtt1t, dtt2t, zero, zerot,
                                  width, tcross);
  }

  return;
}

//------------------------------------------------------------------------------------------------
/** Main function
  * xValues containing the d-space value of peaks centres
  */
void
ThermalNeutronDtoTOFFunction::function1D(vector<double> &out,
                                         const vector<double> xValues) const {
  double dtt1 = getParameter(0);
  double dtt1t = getParameter(1);
  double dtt2t = getParameter(2);
  double zero = getParameter(3);
  double zerot = getParameter(4);
  double width = getParameter(5);
  double tcross = getParameter(6);

  size_t nData = out.size();

  for (size_t i = 0; i < nData; ++i) {
    out[i] = calThermalNeutronTOF(xValues[i], dtt1, dtt1t, dtt2t, zero, zerot,
                                  width, tcross);
  }

  return;
}

void ThermalNeutronDtoTOFFunction::functionDeriv1D(Jacobian *out,
                                                   const double *xValues,
                                                   const size_t nData) {
  // 1. Get hold all parameters
  const double dtt1 = getParameter("Dtt1");
  const double dtt1t = getParameter("Dtt1t");
  const double dtt2t = getParameter("Dtt2t");
  const double zero = getParameter("Zero");
  const double zerot = getParameter("Zerot");
  const double width = getParameter("Width");
  const double tcross = getParameter("Tcross");

  // 2. Calcualtion
  for (size_t i = 0; i < nData; ++i) {
    // a) Some calcualtion
    double x = xValues[i];
    double n = 0.5 * gsl_sf_erfc(width * (tcross - 1 / x));
    double u = width * (tcross - 1 / x);

    double deriv_dtt1 = n * x;
    double deriv_dtt1t = (1 - n) * x;
    double deriv_dtt2t = (n - 1) / x;
    double deriv_zero = n;
    double deriv_zerot = (1 - n);
    double deriv_width = -(zero + dtt1 * x - zerot - dtt1t * x + dtt2t / x) *
                         exp(-u * u) / sqrt(PI) * (tcross - 1 / x);
    double deriv_tcross = -(zero + dtt1 * x - zerot - dtt1t * x + dtt2t / x) *
                          exp(-u * u) / sqrt(PI) * width;

    // b) Set
    out->set(i, 0, deriv_dtt1);
    out->set(i, 1, deriv_dtt1t);
    out->set(i, 2, deriv_dtt2t);
    out->set(i, 3, deriv_zero);
    out->set(i, 4, deriv_zerot);
    out->set(i, 5, deriv_width);
    out->set(i, 6, deriv_tcross);
  }

  return;
}

/** Some forbidden function
  */
void ThermalNeutronDtoTOFFunction::functionDerivLocal(API::Jacobian *,
                                                      const double *,
                                                      const size_t) {
  throw Mantid::Kernel::Exception::NotImplementedError(
      "functionDerivLocal is not implemented for "
      "ThermalNeutronDtoTOFFunction.");
}

} // namespace CurveFitting
} // namespace Mantid
