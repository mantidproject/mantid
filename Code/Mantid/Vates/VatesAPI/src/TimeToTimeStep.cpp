#include "MantidVatesAPI/TimeToTimeStep.h"
#include <stdexcept>

namespace Mantid
{
namespace VATES
{

/// Make construction explicit.
TimeToTimeStep TimeToTimeStep::construct(double timeMin, double timeMax, size_t nIntervalSteps)
{
  return TimeToTimeStep(timeMin, timeMax, nIntervalSteps);
}

TimeToTimeStep::TimeToTimeStep(double timeMin, double timeMax, size_t nIntervalSteps) : m_timeMin(timeMin), m_timeMax(timeMax),
  m_timeRange(timeMax - timeMin), m_nIntervalSteps(nIntervalSteps), m_runnable(true)
{
  if(m_timeRange <= 0)
  {
    throw std::runtime_error("Range must be positive. timeMax should be > timeMin");
  }
}

TimeToTimeStep::TimeToTimeStep(): m_runnable(false)
{
}

size_t TimeToTimeStep::operator()(double time) const
{
  if(!m_runnable)
  {
    throw std::runtime_error("Not properly constructed. TimeToTimeStep instance does not have enough information to interpolate the time value.");
  }
  if(time > m_timeMax)
  {
    //Don't throw, handle by providing the largest time index.
    return 0;
  }
  else if(time < m_timeMin)
  {
    //Don't throw, handle by providing the smallest time index
    return 0;
  }
  else
  {
    return static_cast<size_t>(time / (m_timeRange) * static_cast<double>(m_nIntervalSteps));
  }
}

}
}
