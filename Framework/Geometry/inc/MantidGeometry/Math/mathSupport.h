#ifndef MANTID_MATH_MATHSUPPORT_H_
#define MANTID_MATH_MATHSUPPORT_H_

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Logger.h"
#include <functional>
#include <vector>
#include <complex>

namespace Mantid {

/// Solve a Quadratic equation
template <typename InputIter>
MANTID_GEOMETRY_DLL int solveQuadratic(
    InputIter /*Coef*/,
    std::pair<std::complex<double>, std::complex<double>> & /*OutAns*/);

/// Solve a Cubic equation
template <typename InputIter>
MANTID_GEOMETRY_DLL int
solveCubic(InputIter /*Coef*/, std::complex<double> & /*AnsA*/,
           std::complex<double> & /*AnsB*/, std::complex<double> & /*AnsC*/);

namespace mathSupport {}
}
#endif
