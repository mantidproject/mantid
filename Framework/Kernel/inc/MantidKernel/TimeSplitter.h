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
namespace Kernel {

/**
 * TimeSplitter is an object that contains a mapping of time regions [inclusive, exclusive) that map to output workspace
 * indices. No time can be mapped to two output workspace indices and all time from the beginning to the end is
 * accounted for. A negative workspace index indicates that the data in that region should be ignored. This object
 * converts all negative indices to -1.
 */
class MANTID_KERNEL_DLL TimeSplitter {
public:
  TimeSplitter() = default;
  TimeSplitter(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop);
  int valueAtTime(const Types::Core::DateAndTime &time) const;
  void addROI(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop, const int value);
  std::vector<int> outputWorkspaceIndices() const;
  TimeROI getTimeROI(const int workspaceIndex);

  /// this is to aid in testing and not intended for use elsewhere
  std::size_t numRawValues() const;

private:
  void clearAndReplace(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop, const int value);
  std::string debugPrint() const;
  std::map<Types::Core::DateAndTime, int> m_roi_map;
};

} // namespace Kernel
} // namespace Mantid
