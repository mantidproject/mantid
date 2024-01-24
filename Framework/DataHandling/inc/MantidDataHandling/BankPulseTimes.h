// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidDataHandling/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Property.h"
#include <mutex>

namespace NeXus {
class File;
}

namespace Mantid::DataHandling {
/** This class defines the pulse times for a specific bank.
 * Since some instruments (ARCS, VULCAN) have multiple preprocessors,
 * this means that some banks have different lists of pulse times.
 */
class MANTID_DATAHANDLING_DLL BankPulseTimes {
public:
  /// Starting number for assigning periods.
  static const unsigned int FirstPeriod;

  /// Constructor with NeXus::File
  BankPulseTimes(::NeXus::File &file, const std::vector<int> &periodNumbers);

  /// Constructor with vector of DateAndTime
  BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times);

  /// Constructor with vector of DateAndTime and Period information for testing only
  BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times, const std::vector<int> &periodNumbers);

  /// Threadsafe method to access cached information about sorting
  bool arePulseTimesIncreasing() const;

  /// Equals
  bool equals(size_t otherNumPulse, const std::string &otherStartTime);

  /// String describing the start time
  std::string startTime;

  /// Array of the pulse times
  std::vector<Mantid::Types::Core::DateAndTime> pulseTimes;

  /// Vector of period numbers corresponding to each pulse
  std::vector<int> periodNumbers;

private:
  template <typename ValueType>
  void readData(::NeXus::File &file, int64_t numValues, Mantid::Types::Core::DateAndTime &start);

  /// Ensure that we always have a consistency between nPulses and periodNumbers containers
  void finalizePeriodNumbers();

  mutable int8_t m_sorting_info;
  mutable std::mutex m_sortingMutex;
};
} // namespace Mantid::DataHandling
