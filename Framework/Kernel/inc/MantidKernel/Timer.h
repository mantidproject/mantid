// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include <chrono>
#include <iosfwd>
#include <string>

namespace Mantid {
namespace Kernel {

using time_point_ns = std::chrono::time_point<std::chrono::high_resolution_clock>;

/** A simple class that provides a wall-clock (not processor time) timer.

    @author Russell Taylor, Tessella plc
    @date 29/04/2010
 */
class MANTID_KERNEL_DLL Timer {
public:
  Timer();
  virtual ~Timer() = default;

  float elapsed(bool reset = true);
  float elapsed_no_reset() const;
  std::string str() const;
  void reset();

private:
  time_point_ns m_start; ///< The starting time
};

MANTID_KERNEL_DLL std::ostream &operator<<(std::ostream &, const Timer &);

class MANTID_KERNEL_DLL CodeBlockTimer {
public:
  CodeBlockTimer() = delete;
  CodeBlockTimer(const std::string &name, std::ostream &output);
  ~CodeBlockTimer();

private:
  std::string name;
  std::ostream &out;
  std::chrono::time_point<std::chrono::system_clock> start;
};

class MANTID_KERNEL_DLL CodeBlockMultipleTimer {
public:
  class MANTID_KERNEL_DLL TimeAccumulator {
  public:
    TimeAccumulator() = delete;
    TimeAccumulator(const std::string &name);

  public:
    void reset();
    void increment(const double time_s);
    double getElapsed() const;
    size_t getNumberOfEntrances() const;
    void report(std::ostream &out) const;

  private:
    std::string name;
    double elapsed_s{0.0};
    size_t number_of_entrances{0};
  };

public:
  CodeBlockMultipleTimer() = delete;
  CodeBlockMultipleTimer(TimeAccumulator &accumulator);
  ~CodeBlockMultipleTimer();

private:
  TimeAccumulator &accumulator;
  std::chrono::time_point<std::chrono::system_clock> start;
};

} // namespace Kernel
} // namespace Mantid
