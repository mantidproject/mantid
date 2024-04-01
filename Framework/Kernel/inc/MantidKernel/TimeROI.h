// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2022 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Kernel {

/** TimeROI : Object that holds information about when the time measurement was active.
 */
class MANTID_KERNEL_DLL TimeROI {
public:
  /// the underlying property needs a name
  static const std::string NAME;
  static const TimeROI USE_NONE;
  static const TimeROI USE_ALL;

  TimeROI();
  TimeROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime);
  TimeROI(const Kernel::TimeSeriesProperty<bool> *filter);

  double durationInSeconds() const;
  double durationInSeconds(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) const;
  std::size_t numBoundaries() const;
  std::size_t numberOfRegions() const;
  /// TimeROI selects all time to be used
  bool useAll() const;
  /// TimeROI selects no time to be used as all is invalid
  bool useNone() const;
  void clear();
  void addROI(const std::string &startTime, const std::string &stopTime);
  void addROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime);
  void addROI(const std::time_t &startTime, const std::time_t &stopTime);
  void appendROIFast(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime);
  void addMask(const std::string &startTime, const std::string &stopTime);
  void addMask(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime);
  void addMask(const std::time_t &startTime, const std::time_t &stopTime);
  bool valueAtTime(const Types::Core::DateAndTime &time) const;
  Types::Core::DateAndTime getEffectiveTime(const Types::Core::DateAndTime &time) const;
  Types::Core::DateAndTime firstTime() const;
  Types::Core::DateAndTime lastTime() const;
  const std::vector<Types::Core::DateAndTime> &getAllTimes() const { return m_roi; }

  void replaceROI(const TimeSeriesProperty<bool> *roi);
  void replaceROI(const TimeROI &other);
  void replaceROI(const std::vector<Types::Core::DateAndTime> &roi);

  void update_union(const TimeROI &other);
  void update_intersection(const TimeROI &other);
  void update_or_replace_intersection(const TimeROI &other);
  const std::vector<Kernel::TimeInterval> toTimeIntervals() const;
  const std::vector<Kernel::TimeInterval> toTimeIntervals(const Types::Core::DateAndTime &after) const;
  bool operator==(const TimeROI &other) const;
  bool operator!=(const TimeROI &other) const;
  /// print the ROI boundaries to a string
  std::string debugStrPrint(const std::size_t type = 0) const;
  size_t getMemorySize() const;
  Types::Core::DateAndTime timeAtIndex(unsigned long index) const;

  // nexus items
  void saveNexus(::NeXus::File *file) const;

private:
  std::vector<Types::Core::DateAndTime> getAllTimes(const TimeROI &other);
  void validateValues(const std::string &label);
  bool empty() const;
  bool isCompletelyInROI(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) const;
  bool isCompletelyInMask(const Types::Core::DateAndTime &startTime, const Types::Core::DateAndTime &stopTime) const;
  std::vector<Types::Core::DateAndTime> m_roi;
};

namespace ROI {
/**
 * This calculates the intersection of two sorted vectors that represent regions of interest (ROI).
 * The ROI are pairs of [[include, exclude), [include,exclude), ...] where an empty vector is interpreted to mean "use
 * all".
 */
template <typename TYPE>
std::vector<TYPE> calculate_intersection(const std::vector<TYPE> &left, const std::vector<TYPE> &right);
} // namespace ROI

} // namespace Kernel
} // namespace Mantid
