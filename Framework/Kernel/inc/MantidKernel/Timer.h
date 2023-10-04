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

} // namespace Kernel
} // namespace Mantid

namespace {
class CumulativeTimer {
public:
  CumulativeTimer(const std::string &timer_name) { name = timer_name; }

public:
  void incrementTimeSpent(const double time_s) { total_time_spent_s += time_s; }
  void reportTiming() { std::cout << "Cumulative time (s) spent in " << name << ": " << total_time_spent_s << "\n"; }

private:
  std::string name;
  double total_time_spent_s = 0.0;
};

// std::shared_ptr<CumulativeTimer> copyAndFilterProperties_Timer =
//     std::make_shared<CumulativeTimer>("LogManager::CopyAndFilterProperties");
// std::shared_ptr<CumulativeTimer> cloneInTimeROI_Timer =
// std::make_shared<CumulativeTimer>("LogManager::cloneInTimeROI"); std::shared_ptr<CumulativeTimer> setTimeROI_Timer =
// std::make_shared<CumulativeTimer>("LogManager::setTimeROI"); std::shared_ptr<CumulativeTimer>
// clearSingleValueCache_Timer =
//     std::make_shared<CumulativeTimer>("LogManager::clearSingleValueCache");
// std::shared_ptr<CumulativeTimer> m_manager_cloneInTimeROI_Timer =
//     std::make_shared<CumulativeTimer>("m_manager->cloneInTimeROI");
// std::shared_ptr<CumulativeTimer> outputTimeROI_update_or_replace_intersection_Timer =
//     std::make_shared<CumulativeTimer>("outputTimeROI_update_or_replace_intersection_Timer");

// The constructor saves the start time, the destructor calculates the elapsed time
class CodeBlockTimer {
public:
  CodeBlockTimer(std::shared_ptr<CumulativeTimer> cumulativeTimer) {
    cumulative_timer = cumulativeTimer;
    start = std::chrono::system_clock::now();
  }
  ~CodeBlockTimer() {
    std::chrono::time_point<std::chrono::system_clock> stop = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = stop - start;
    cumulative_timer->incrementTimeSpent(elapsed_seconds.count());
  }

private:
  std::shared_ptr<CumulativeTimer> cumulative_timer;
  std::chrono::time_point<std::chrono::system_clock> start;
};
} // namespace
