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

// Computes differential of the A_z formula for fitting functions for decoupling of asymmetry in the ordered state of a
// powdered magnet
double MANTID_CURVEFITTING_DLL getDiffAz(double xValue, const double charField);

// Computes the Activation Fit function
double MANTID_CURVEFITTING_DLL getActivationFunc(double xValue, const double attemptRate, const double barrier,
                                                 const double unitMultipy);

// Computes the Activation Fit function differentials
double MANTID_CURVEFITTING_DLL getAttemptRateDiff(double xValue, const double barrier, const double unitMultipy);
double MANTID_CURVEFITTING_DLL getBarrierDiff(double xValue, const double attemptRate, const double barrier,
                                              const double unitMultipy);
} // namespace MuonHelper
} // namespace CurveFitting
} // namespace Mantid
