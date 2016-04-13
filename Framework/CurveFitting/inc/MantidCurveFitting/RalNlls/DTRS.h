#ifndef CURVEFITTING_RAL_NLLS_DTRST_H_
#define CURVEFITTING_RAL_NLLS_DTRST_H_

namespace Mantid {
namespace CurveFitting {
namespace RalNlls {

inline double sign(double x, double y) {
  return y >= 0.0 ? std::abs(x) : -std::abs(x);
};

/// Find roots of a quadratic equation.
void roots_quadratic(double a0, double a1, double a2, double tol, int nroots, 
                     double& root1, double &root2, bool debug );
}
}
}

#endif // CURVEFITTING_RAL_NLLS_DTRS_H_
