// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Kernel {

namespace {
// const std::string NAME{"roi"};
}

/** TimeROI : TODO: DESCRIPTION
 */
class MANTID_KERNEL_DLL TimeROI {
public:
  TimeROI();
  TimeROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime);
  double durationInSeconds() const;
  std::size_t numBoundaries() const;
  bool empty() const;
  void addROI(const std::string &startTime, const std::string &stopTime);
  void addROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime);
  void addROI(const std::time_t &startTime, const std::time_t &stopTime);
  void addMask(const std::string &startTime, const std::string &stopTime);
  void addMask(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime);
  void addMask(const std::time_t &startTime, const std::time_t &stopTime);
  bool valueAtTime(const Types::Core::DateAndTime &time) const;
  /// https://en.wikipedia.org/wiki/Union_(set_theory)
  void update_union(const TimeROI &other);
  /// https://en.wikipedia.org/wiki/Intersection
  void update_intersection(const TimeROI &other);
  void removeRedundantEntries();
  bool operator==(const TimeROI &other) const;
  void debugPrint() const;

private:
  /**
   * @brief m_roi private member that holds most of the information
   *
   * This handles the details of the ROI and guarantees that views into the underlying structure is sorted by time.
   */
  TimeSeriesProperty<bool> m_roi;
};

} // namespace Kernel
} // namespace Mantid
