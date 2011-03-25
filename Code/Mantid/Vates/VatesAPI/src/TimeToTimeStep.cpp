#include "MantidVatesAPI/TimeToTimeStep.h"
#include <stdexcept>

namespace Mantid
{
namespace VATES
{

TimeToTimeStep::TimeToTimeStep(double timeMin, double timeMax, unsigned int nIntervalSteps) : m_timeMin(timeMin), m_timeMax(timeMax),
  m_timeRange(timeMax - timeMin), m_nIntervalSteps(nIntervalSteps)
{
  if(m_timeRange <= 0)
  {
    throw std::runtime_error("Range must be positive. timeMax should be > timeMin");
  }
}

int TimeToTimeStep::operator()(double time) const
{
  if(time > m_timeMax)
  {
    //Don't throw, handle by providing the larget time index. 
    return 0;
  }
  else if(time < m_timeMin)
  {
    //Don't throw, handle by providing the smallest time index
    return 0;
  }
  else
  {
    return (int) ((time / (m_timeRange)) * m_nIntervalSteps);
  }
}

}
}
