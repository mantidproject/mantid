//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Timer.h"

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
#ifdef _WIN32
  clock_t now = clock();
  const float retval = float(now - m_start)/CLOCKS_PER_SEC;
  m_start = now;
#else /* linux & mac */
  timeval now;
  gettimeofday(&now,0);
  const float retval = float(now.tv_sec - m_start.tv_sec) + float((now.tv_usec - m_start.tv_usec)/1E6);
  m_start = now;
#endif
  
  return retval;
}

} // namespace Kernel
} // namespace Mantid
