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
  buffer << this->elapsed_no_reset() << " sec";
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
    : m_name(name), m_out(out), m_start(std::chrono::system_clock::now()) {}

/** Calculate and output to a stream the elapsed wall-clock time (sec)
 */
CodeBlockTimer::~CodeBlockTimer() {
  const auto stop = std::chrono::system_clock::now();
  const std::chrono::duration<double> elapsed = stop - m_start;
  m_out << "Elapsed time (sec) in \"" << m_name << "\": " << elapsed.count() << '\n';
}

//------------------------------------------------------------------------
/** Instantiate the object and start the timer
 * @param accumulator :: a persistent object keeping track of the total elapsed wall-clock time
 */
CodeBlockMultipleTimer::CodeBlockMultipleTimer(CodeBlockMultipleTimer::TimeAccumulator &accumulator)
    : m_accumulator(accumulator), m_start(std::chrono::system_clock::now()) {}

/** Calculate the elapsed wall-clock time (seconds) and update the time accumulator
 */
CodeBlockMultipleTimer::~CodeBlockMultipleTimer() {
  const auto stop = std::chrono::system_clock::now();
  const std::chrono::duration<double> elapsed = stop - m_start;
  m_accumulator.increment(elapsed.count());
}

//------------------------------------------------------------------------
/** Instantiate the object
 * @param name :: custom name of the code block
 */
CodeBlockMultipleTimer::TimeAccumulator::TimeAccumulator(const std::string &name) : m_name(name) {}

/** Reset the elapsed wall-clock time and number of times the code block was entered
 */
void CodeBlockMultipleTimer::TimeAccumulator::reset() {
  m_elapsed_sec = 0.0;
  m_number_of_entrances = 0;
}

/** Increment the elapsed wall-clock time and number of times the code block was entered
 * @param time_sec :: elapsed time (seconds) to add
 */
void CodeBlockMultipleTimer::TimeAccumulator::increment(const double time_sec) {
  m_elapsed_sec += time_sec;
  m_number_of_entrances++;
}

/** Return the total elapsed wall-clock time
 * @return :: elapsed time in seconds
 */
double CodeBlockMultipleTimer::TimeAccumulator::getElapsed() const { return m_elapsed_sec; }

/** Return the number of times the code block was entered
 * @return :: number of entrances
 */
size_t CodeBlockMultipleTimer::TimeAccumulator::getNumberOfEntrances() const { return m_number_of_entrances; }

/** Return the timing summary as a string
 * @return :: timing summary
 */
std::string CodeBlockMultipleTimer::TimeAccumulator::toString() const {
  std::ostringstream out;
  out << "Elapsed time (sec) in \"" << m_name << "\": " << m_elapsed_sec
      << "; Number of entrances: " << m_number_of_entrances;
  return out.str();
}

/** Output timing summary to a stream
 * @param out :: stream to output the timing summary
 * @param ta :: time accumulator keeping the timing summary
 * @return :: stream to output the timing summary
 */
MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &out, const CodeBlockMultipleTimer::TimeAccumulator &ta) {
  out << ta.toString();
  return out;
}
} // namespace Mantid::Kernel
