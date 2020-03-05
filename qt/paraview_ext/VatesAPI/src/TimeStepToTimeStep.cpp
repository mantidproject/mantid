// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidVatesAPI/TimeStepToTimeStep.h"

namespace Mantid {
namespace VATES {
TimeStepToTimeStep::TimeStepToTimeStep(double timeMin, double timeMax,
                                       size_t intervalStep) {
  UNUSED_ARG(timeMin);
  UNUSED_ARG(timeMax);
  UNUSED_ARG(intervalStep);
}

TimeStepToTimeStep TimeStepToTimeStep::construct(double timeMin, double timeMax,
                                                 size_t nIntervalSteps) {
  return TimeStepToTimeStep(timeMin, timeMax, nIntervalSteps);
}

size_t TimeStepToTimeStep::operator()(double timeStep) const {
  return static_cast<size_t>(timeStep);
}
} // namespace VATES
} // namespace Mantid
