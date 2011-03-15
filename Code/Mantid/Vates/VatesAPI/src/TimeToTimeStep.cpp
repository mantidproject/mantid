#include "MantidVatesAPI/TimeToTimeStep.h"
#include <stdexcept>

namespace Mantid
{
namespace VATES
{

TimeToTimeStep::TimeToTimeStep(double timeMin, double timeMax, unsigned int nIntervalSteps) :
  m_timeRange(timeMax - timeMin), m_nIntervalSteps(nIntervalSteps)
{
  if(m_timeRange <= 0)
  {
    throw std::runtime_error("Range must be positive. timeMax should be > timeMin");
  }
}

int TimeToTimeStep::operator()(double time) const
{
  return (int) ((time / (m_timeRange)) * m_nIntervalSteps);
}

}
}
