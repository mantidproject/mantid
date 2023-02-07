// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DllConfig.h"

namespace Mantid {
namespace Kernel {

/** TimeSplitter : TODO: DESCRIPTION
 */
class MANTID_KERNEL_DLL TimeSplitter {
public:
  TimeSplitter() = default;
  TimeSplitter(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop);
  int valueAtTime(const Types::Core::DateAndTime &time) const;
  void addROI(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop, const int value);
  std::vector<int> outputWorkspaceIndices() const;

  /// this is to aid in testing and not intended for use elsewhere
  std::size_t numRawValues() const;

private:
  void clearAndReplace(const Types::Core::DateAndTime &start, const Types::Core::DateAndTime &stop, const int value);
  std::map<Types::Core::DateAndTime, int> m_roi_map;
};

} // namespace Kernel
} // namespace Mantid
