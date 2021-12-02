// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidCurveFitting/DllConfig.h"
#include <cmath>

namespace Mantid {
namespace CurveFitting {
namespace MuonHelper {
// Computes A_z formula for fitting functions for decoupling of asymmetry in the ordered state of a powdered magnet
double MANTID_CURVEFITTING_DLL getAz(double xValue, const double charField);

} // namespace MuonHelper
} // namespace CurveFitting
} // namespace Mantid
