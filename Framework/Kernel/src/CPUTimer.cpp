// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/CPUTimer.h"
#include <ctime>
#include <iomanip>
#include <sstream>

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------------------------------
/** Constructor
 */
CPUTimer::CPUTimer() {
  // Record the starting time
  reset();
}

/// Explicitly reset the timer.
void CPUTimer::reset() {
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
float CPUTimer::elapsedCPU(bool doReset) {
  float retval = 0;
#ifdef _WIN32
  UNUSED_ARG(doReset);
#else /* linux & mac */
  clock_t end = clock();
  retval = (static_cast<float>(end - m_start)) / CLOCKS_PER_SEC;
  if (doReset)
    this->reset();
#endif
  return retval;
}

/** Calculate the elapsed wall-clock time, reseting the timer if specified
 *
 * @param doReset :: true to reset the timer
 * @return wall-clock time, in seconds
 */
float CPUTimer::elapsedWallClock(bool doReset) {
  float retVal = m_wallClockTime.elapsed(false);
  if (doReset)
    this->reset();
  return retVal;
}

/** Return the fraction of the CPU used (CPUTime/wall-clock time).
 * This can be > 1 on multi-CPU systems.
 *
 * @param doReset :: true to reset both timers
 * @return
 */
float CPUTimer::CPUfraction(bool doReset) {
  // Get the wall-clock time without resetting.
  double wallTime = m_wallClockTime.elapsed(false);
  double cpuTime = elapsedCPU(false);
  if (doReset)
    this->reset();
  return static_cast<float>((cpuTime / wallTime));
}

/// Convert the elapsed time (without reseting) to a string.
std::string CPUTimer::str() {
  std::stringstream buffer;
  buffer << std::fixed << std::setw(7) << std::setprecision(4)
         << m_wallClockTime.elapsed_no_reset() << " s, CPU "
         << std::setprecision(2) << this->CPUfraction(false);
  this->reset();
  return buffer.str();
}

/// Convenience function to provide for easier debug printing.
std::ostream &operator<<(std::ostream &out, CPUTimer &obj) {
  out << obj.str();
  return out;
}

} // namespace Kernel
} // namespace Mantid
