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
MANTID_GEOMETRY_DLL int
solveQuadratic(InputIter,
               std::pair<std::complex<double>, std::complex<double>> &);

/// Solve a Cubic equation
template <typename InputIter>
MANTID_GEOMETRY_DLL int solveCubic(InputIter, std::complex<double> &,
                                   std::complex<double> &,
                                   std::complex<double> &);

namespace mathSupport {}
}
#endif
