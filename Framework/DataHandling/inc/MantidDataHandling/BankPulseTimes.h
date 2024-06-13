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
  static const int FIRST_PERIOD;
  /// Unix epoch used as default epoch when the file does not specify one
  static const std::string DEFAULT_START_TIME;

  /// Constructor with NeXus::File
  BankPulseTimes(::NeXus::File &file, const std::vector<int> &periodNumbers);

  /// Constructor with vector of DateAndTime
  BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times);

  /// Constructor with vector of DateAndTime and Period information for testing only
  BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times, const std::vector<int> &periodNumbers);

  /// The number of pulses being held
  size_t numberOfPulses() const;

  /// Threadsafe method to access cached information about sorting
  bool arePulseTimesIncreasing() const;

  /// The raw number from the period stored in the logs
  int periodNumber(const size_t index) const;
  /// The wall clock time of the pulse
  const Mantid::Types::Core::DateAndTime &pulseTime(const size_t index) const;

  /**
   * Return a vector of [include,exclude) indices into the pulse vectors that are between the start and stop times. This
   * is very similar to the behavior of Mantid::Kernel::TimeROI.
   *
   * This will return an empty vector if all pulse indices are between the start and stop.
   */
  std::vector<size_t> getPulseIndices(const Mantid::Types::Core::DateAndTime &start,
                                      const Mantid::Types::Core::DateAndTime &stop) const;

  // convert a list of time intervals to pulse indices
  std::vector<size_t> getPulseIndices(const std::vector<Mantid::Kernel::TimeInterval> &splitters) const;

  /// Equals
  bool equals(size_t otherNumPulse, const std::string &otherStartTime);

  /// String describing the start time
  std::string startTime;

private:
  template <typename ValueType>
  void readData(::NeXus::File &file, int64_t numValues, Mantid::Types::Core::DateAndTime &start);

  /// This determines the start time by finding the minimum value in the array
  void updateStartTime();

  /// Ensure that we always have a consistency between nPulses and periodNumbers containers
  void finalizePeriodNumbers();

  /// Vector of period numbers corresponding to each pulse
  std::vector<int> periodNumbers;

  /// Array of the pulse times
  std::vector<Mantid::Types::Core::DateAndTime> pulseTimes;

  bool have_period_info;

  mutable int8_t m_sorting_info;
  mutable std::mutex m_sortingMutex;
};
} // namespace Mantid::DataHandling
