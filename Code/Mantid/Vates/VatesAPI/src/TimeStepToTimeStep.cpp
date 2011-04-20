#include "MantidVatesAPI/TimeStepToTimeStep.h"

namespace Mantid
{
namespace VATES
{
TimeStepToTimeStep::TimeStepToTimeStep(double timeMin, double timeMax, size_t intervalStep)
{
  UNUSED_ARG(timeMin);
  UNUSED_ARG(timeMax);
  UNUSED_ARG(intervalStep);
}

TimeStepToTimeStep TimeStepToTimeStep::construct(double timeMin, double timeMax, size_t nIntervalSteps)
{
  return TimeStepToTimeStep(timeMin, timeMax, nIntervalSteps);
}

TimeStepToTimeStep::TimeStepToTimeStep()
{
}

int TimeStepToTimeStep::operator()(int timeStep) const
{
  return timeStep;
}
}
}
