// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2021 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidCurveFitting/Functions/ActivationmeV.h"
#include "MantidAPI/FunctionFactory.h"
#include "MantidCurveFitting/MuonHelpers.h"
#include "MantidKernel/PhysicalConstants.h"

#include <cmath>

namespace Mantid::CurveFitting::Functions {

using namespace CurveFitting;

using namespace Kernel;

using namespace API;

using namespace CurveFitting::MuonHelper;

DECLARE_FUNCTION(ActivationmeV)

void ActivationmeV::init() {
  declareParameter("AttemptRate", 1000.0, "coefficient for attempt rate");
  declareParameter("Barrier", 1000.0, "coefficient for barrier energy");
}

void ActivationmeV::function1D(double *out, const double *xValues, const size_t nData) const {
  const double attemptRate = getParameter("AttemptRate");
  const double barrier = getParameter("Barrier");
  const double meVConv = PhysicalConstants::meVtoKelvin;

  for (size_t i = 0; i < nData; i++) {
    out[i] = getActivationFunc(xValues[i], attemptRate, barrier, meVConv);
  }
}

void ActivationmeV::functionDeriv1D(Jacobian *out, const double *xValues, const size_t nData) {
  const double attemptRate = getParameter("AttemptRate");
  const double barrier = getParameter("Barrier");
  const double meVConv = PhysicalConstants::meVtoKelvin;

  for (size_t i = 0; i < nData; i++) {
    double diffAR = getAttemptRateDiff(xValues[i], barrier, meVConv);
    double diffBarrier = getBarrierDiff(xValues[i], attemptRate, barrier, meVConv);
    out->set(i, 0, diffAR);
    out->set(i, 1, diffBarrier);
  }
}

} // namespace Mantid::CurveFitting::Functions
