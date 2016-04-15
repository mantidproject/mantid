#ifndef CURVEFITTING_RAL_NLLS_DTRST_H_
#define CURVEFITTING_RAL_NLLS_DTRST_H_

#include "MantidCurveFitting/DllConfig.h"

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

/// Replacement for FORTRAN's SIGN intrinsic function
inline double sign(double x, double y) {
  return y >= 0.0 ? std::abs(x) : -std::abs(x);
};

/// Find roots of a quadratic equation.
MANTID_CURVEFITTING_DLL void roots_quadratic(double a0, double a1, double a2, double tol, int &nroots, 
                     double& root1, double &root2, bool debug = false );

/// Find roots of a cubic equation.
MANTID_CURVEFITTING_DLL void roots_cubic(double a0, double a1, double a2, double a3, double tol, int &nroots, 
                     double& root1, double &root2, double &root3, bool debug = false );

/// Find roots of a quartic equation.
MANTID_CURVEFITTING_DLL void roots_quartic(double a0, double a1, double a2, double a3, double a4, double tol, int &nroots, 
                     double& root1, double &root2, double &root3, double &root4, bool debug = false);


}
}
}

#endif // CURVEFITTING_RAL_NLLS_DTRS_H_
