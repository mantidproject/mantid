#include "MantidVatesAPI/TimeStepToTimeStep.h"

namespace Mantid
{
namespace VATES
{
TimeStepToTimeStep::TimeStepToTimeStep(){}

int TimeStepToTimeStep::operator()(int timeStep) const
{
  return timeStep;
}
}
}
