#include "MantidVatesAPI/TimeToTimeStep.h"
#include <stdexcept>

namespace Mantid
{
namespace VATES
{

/**
  Constructional method.
  @return a fully constructed TimeToTimestep instance.
*/
TimeToTimeStep TimeToTimeStep::construct(double timeMin, double timeMax, size_t nIntervalSteps)
{
  return TimeToTimeStep(timeMin, timeMax, nIntervalSteps);
}

/**
  Constructor.
  @param timeMin : the minimum time/parameter in the range.
  @param timeMax : the maximum time/parameter in the range.
  @param nIntervalSteps : the number of interval steps available.
*/
TimeToTimeStep::TimeToTimeStep(double timeMin, double timeMax, size_t nIntervalSteps) : m_timeMin(timeMin), m_timeMax(timeMax),
  m_timeRange(timeMax - timeMin), m_nIntervalSteps(nIntervalSteps), m_runnable(true)
{
  if(m_timeRange <= 0)
  {
    throw std::runtime_error("Range must be positive. timeMax should be > timeMin");
  }
  //This fraction is convenient to pre-calculate.
  m_fraction = (1 / m_timeRange) * static_cast<double>(m_nIntervalSteps);
  //The offset is scoped for the functor lifetime.
  m_c = -1 * m_fraction * m_timeMin; 
}

/**
  Default constructor.
*/
TimeToTimeStep::TimeToTimeStep(): m_timeMin(0.0), m_timeMax(0.0), m_timeRange(0.0),
  m_c(0.0), m_fraction(0.0), m_nIntervalSteps(0), m_runnable(false)
{
}

/**
  Overloaded funtion operator. 
  @param time : time/property value wrt max and min property values of this type.
  @return a calculated index for the time/property value provided.
*/
size_t TimeToTimeStep::operator()(double time) const
{
  if(!m_runnable)
  {
    throw std::runtime_error("Not properly constructed. TimeToTimeStep instance does not have enough information to interpolate the time value.");
  }
  if(time > m_timeMax || time < m_timeMin)
  {
    return 0;
  }
  else
  {
    return static_cast<size_t>((time * m_fraction) + m_c);  //Linear interpolation
  }
}

}
}
