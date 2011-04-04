#include "MantidKernel/CPUTimer.h"
#include "MantidKernel/System.h"
#include <ctime>
#include <sstream>

namespace Mantid
{
namespace Kernel
{


  //----------------------------------------------------------------------------------------------
  /** Constructor
   */
  CPUTimer::CPUTimer()
  {
    // Record the starting time
    reset();
  }
    
  //----------------------------------------------------------------------------------------------
  /** Destructor
   */
  CPUTimer::~CPUTimer()
  {
  }
  

  /// Explicitly reset the timer.
  void CPUTimer::reset()
  {
  #ifdef _WIN32
  #else /* linux & mac */
    m_start = clock();
  #endif
    m_wallClockTime.reset();
  }

  /** Calculate the elapsed CPU time, reseting the timer if specified
   *
   * @param doReset :: true to reset the timer
   * @return time in CPU seconds
   */
  float CPUTimer::elapsed(bool doReset)
  {
    float retval = 0;
  #ifdef _WIN32
  #else /* linux & mac */
    clock_t end = clock();
    retval = ((float)(end - m_start)) / CLOCKS_PER_SEC;
    if (doReset) this->reset();
  #endif
    return retval;
  }

  /** Return the fraction of the CPU used (CPUTime/wall-clock time).
   * This can be > 1 on multi-CPU systems.
   *
   * @param reset :: true to reset both timers
   * @return
   */
  float CPUTimer::CPUfraction(bool doReset)
  {
    // Get the wall-clock time without resetting.
    double wallTime = m_wallClockTime.elapsed(false);
    double cpuTime = elapsed(false);
    if (doReset) this->reset();
    return (cpuTime / wallTime);
  }




} // namespace Mantid
} // namespace Kernel

