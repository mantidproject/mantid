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
Timer::Timer()
{
#ifdef _WIN32
  m_start = clock();
#else /* linux & mac */
  gettimeofday(&m_start,0);
#endif
}

/// Destructor
Timer::~Timer()
{}

/// Returns the wall-clock time elapsed in seconds since the Timer object's creation, or the last call to elapsed
float Timer::elapsed()
{
  float retval = this->elapsed_no_reset();
  this->reset();
  return retval;
}

/// Explicitly reset the timer.
void Timer::reset()
{
#ifdef _WIN32
  m_start = clock();
#else /* linux & mac */
  timeval now;
  gettimeofday(&now,0);
  m_start = now;
#endif
}

/// Calculate the elapsed time without reseting the timer.
float Timer::elapsed_no_reset() const
{
#ifdef _WIN32
  clock_t now = clock();
  const float retval = float(now - m_start)/CLOCKS_PER_SEC;
#else /* linux & mac */
  timeval now;
  gettimeofday(&now,0);
  const float retval = float(now.tv_sec - m_start.tv_sec) + float((now.tv_usec - m_start.tv_usec)/1E6);
#endif
  return retval;
}

/// Convert the elapsed time (without reseting) to a string.
std::string Timer::str() const
{
  std::stringstream buffer;
  buffer << this->elapsed_no_reset() << "s";
  return buffer.str();
}

/// Convenience function to provide for easier debug printing.
std::ostream& operator<<(std::ostream& out, const Timer& obj)
{
  out << obj.str();
  return out;
}

} // namespace Kernel
} // namespace Mantid
