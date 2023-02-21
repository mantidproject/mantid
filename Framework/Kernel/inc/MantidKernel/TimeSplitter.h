// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/TimeROI.h"

namespace Mantid {
namespace DataObjects {
// Forward declarations
class EventList
} // namespace DataObjects
namespace Kernel {

/**
 * TimeSplitter is an object that contains a mapping of time regions [inclusive, exclusive) that map to output workspace
 * indices. No time can be mapped to two output workspace indices and all time from the beginning to the end is
 * accounted for. A negative workspace index indicates that the data in that region should be ignored. This object
 * converts all negative indices to -1.
 *
 * Example: below there's a graphic representation of five (DateAndTime, int) pairs.
 *
 * --[--i=1---)[--i=0----)[--i=-1----)[--i=1-----)[--i=0-----> (DateAndTime axis)
 *  t_0      t_1        t_2         t_3         t_4
 *
 *  Any time t < t_0  is associated to destination index -1 (implicit assumption).
 *  Any time t_0 <= t < t_1 is associated to destination index 1.
 *  Any time t_4 <= t is associated to destination index 0.
 */
class MANTID_KERNEL_DLL TimeSplitter {
public:
  TimeSplitter() = default;
  TimeSplitter(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop);
  int valueAtTime(const Types::Core::DateAndTime &time) const;
  void addROI(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop, const int value);
  std::vector<int> outputWorkspaceIndices() const;
  TimeROI getTimeROI(const int workspaceIndex);
  // Split a list of events according to Pulse time
  void splitEvents(const DataObjects::EventList &events, std::map<int, DataObjects::EventList *> partials) const;
  // Split a list of events according to Pulse+TOF time
  void splitEvents(const DataObjects::EventList &events, std::map<int, DataObjects::EventList *> partials,
                   bool tofCorrect, double factor = 1.0, double shift = 0.0) const;

  /// this is to aid in testing and not intended for use elsewhere
  std::size_t numRawValues() const;

private:
  void clearAndReplace(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop, const int value);
  std::string debugPrint() const;
  std::map<Types::Core::DateAndTime, int> m_roi_map;
};

} // namespace Kernel
} // namespace Mantid
