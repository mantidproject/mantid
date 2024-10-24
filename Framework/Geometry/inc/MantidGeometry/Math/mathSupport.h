// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Logger.h"
#include <complex>
#include <functional>
#include <vector>

namespace Mantid {

/// Solve a Quadratic equation
template <typename InputIter>
MANTID_GEOMETRY_DLL int solveQuadratic(InputIter, std::pair<std::complex<double>, std::complex<double>> &);

/// Solve a Cubic equation
template <typename InputIter>
MANTID_GEOMETRY_DLL int solveCubic(InputIter, std::complex<double> &, std::complex<double> &, std::complex<double> &);

namespace mathSupport {}
} // namespace Mantid
