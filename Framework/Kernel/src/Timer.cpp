// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/Timer.h"
#include <chrono>
#include <ostream>
#include <sstream>

namespace Mantid::Kernel {

/** Constructor.
 *  Instantiating the object starts the timer.
 */
Timer::Timer() : m_start(std::chrono::high_resolution_clock::now()) {}

/** Returns the wall-clock time elapsed in seconds since the Timer object's
 *creation, or the last call to elapsed
 *
 * @param reset :: set to true to reset the clock (default)
 * @return time in seconds
 */
float Timer::elapsed(bool reset) {
  float retval = elapsed_no_reset();
  if (reset)
    this->reset();
  return retval;
}

/** Returns the wall-clock time elapsed in seconds since the Timer object's
 *creation, or the last call to elapsed
 *
 * @return time in seconds
 */
float Timer::elapsed_no_reset() const {
  const auto now = std::chrono::high_resolution_clock::now();
  std::chrono::duration<float> duration = now - m_start;

  return duration.count();
}

/// Explicitly reset the timer.
void Timer::reset() { m_start = std::chrono::high_resolution_clock::now(); }

/// Convert the elapsed time (without reseting) to a string.
std::string Timer::str() const {
  std::stringstream buffer;
  buffer << this->elapsed_no_reset() << "s";
  return buffer.str();
}

/// Convenience function to provide for easier debug printing.
std::ostream &operator<<(std::ostream &out, const Timer &obj) {
  out << obj.str();
  return out;
}

//------------------------------------------------------------------------
/** Instantiate the object and start the timer
 * @param name :: custom name of the code block
 * @param out :: stream to output the elapsed wall-clock time
 */
CodeBlockTimer::CodeBlockTimer(const std::string &name, std::ostream &out)
    : name(name), out(out), start(std::chrono::system_clock::now()) {}

/** Calculate and output the elapsed wall-clock time
 */
CodeBlockTimer::~CodeBlockTimer() {
  const auto stop = std::chrono::system_clock::now();
  const std::chrono::duration<double> elapsed = stop - start;
  out << "Elapsed time (s) in \"" << name << "\": " << elapsed.count() << '\n';
}

//------------------------------------------------------------------------
/** Instantiate the object and start the timer
 * @param accumulator :: a persistent object keeping track of the total elapsed wall-clock time
 */
CodeBlockTimerMultipleUse::CodeBlockTimerMultipleUse(CodeBlockTimerMultipleUse::TimeAccumulator &accumulator)
    : accumulator(accumulator), start(std::chrono::system_clock::now()) {}

/** Calculate the elapsed wall-clock time and update the time accumulator
 */
CodeBlockTimerMultipleUse::~CodeBlockTimerMultipleUse() {
  const auto stop = std::chrono::system_clock::now();
  const std::chrono::duration<double> elapsed = stop - start;
  accumulator.incrementElapsed(elapsed.count());
}

//------------------------------------------------------------------------
/** Instantiate the object
 * @param name :: custom name of the code block
 */
CodeBlockTimerMultipleUse::TimeAccumulator::TimeAccumulator(const std::string &name) : name(name) {}

/** Reset the elapsed wall-clock time
 */
void CodeBlockTimerMultipleUse::TimeAccumulator::reset() { elapsed_s = 0.0; }

/** Increment the elapsed wall-clock time
 * @param time_s :: elapsed time (seconds) to add
 */
void CodeBlockTimerMultipleUse::TimeAccumulator::incrementElapsed(const double time_s) { elapsed_s += time_s; }

/** Return the elapsed time
 * @return :: elapsed time
 */
double CodeBlockTimerMultipleUse::TimeAccumulator::getElapsed() const { return elapsed_s; }

/** Output the elapsed time
 * @param out :: stream to output the elapsed time
 */
void CodeBlockTimerMultipleUse::TimeAccumulator::outputElapsed(std::ostream &out) const {
  out << "Cumulative elapsed time (s) in \"" << name << "\": " << elapsed_s << '\n';
}

} // namespace Mantid::Kernel
