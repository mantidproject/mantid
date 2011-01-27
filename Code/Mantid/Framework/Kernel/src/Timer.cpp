//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Timer.h"
#include <sstream>

namespace Mantid
{
namespace Kernel
{

/** Constructor.
 *  Instantiating the object starts the timer.
 */
Timer::Timer(): m_resets(true)
{
  this->m_timer = boost::timer();
}

/// Destructor
Timer::~Timer()
{}

/// Manually reset the timer.
void Timer::restart()
{
  this->m_timer.restart();
}

/// Turns off the timer being reset.
void Timer::resets(const bool resets)
{
  this->m_resets = resets;
}

/// Returns the wall-clock time elapsed in seconds since the Timer object's creation, or the last call to elapsed
double Timer::elapsed()
{
  double result = this->m_timer.elapsed();
  if (this->m_resets)
    this->m_timer.restart();
  return result;
}

std::string Timer::str() const
{
  std::stringstream buffer;
  buffer << this->m_timer.elapsed() << "s";
  return buffer.str();
}

std::ostream& operator<<(std::ostream& out, const Timer obj)
{
  out << obj.str();
  return out;
}

} // namespace Kernel
} // namespace Mantid
