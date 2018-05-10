#ifndef MANTID_KERNEL_BANKPULSETIMES_H
#define MANTID_KERNEL_BANKPULSETIMES_H

#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/Property.h"

#include <nexus/NeXusFile.hpp>

using namespace ::NeXus;

/** This class defines the pulse times for a specific bank.
 * Since some instruments (ARCS, VULCAN) have multiple preprocessors,
 * this means that some banks have different lists of pulse times.
 */
class BankPulseTimes {
public:
  /// Starting number for assigning periods.
  static const unsigned int FirstPeriod;

  /// Constructor with NeXus::File
  BankPulseTimes(::NeXus::File &file, const std::vector<int> &pNumbers);

  /// Constructor with vector of DateAndTime
  BankPulseTimes(const std::vector<Mantid::Types::Core::DateAndTime> &times);

  /// Destructor
  ~BankPulseTimes();

  /// Equals
  bool equals(size_t otherNumPulse, std::string otherStartTime);

  /// String describing the start time
  std::string startTime;

  /// Size of the array of pulse times
  size_t numPulses;

  /// Array of the pulse times
  Mantid::Types::Core::DateAndTime *pulseTimes;

  /// Vector of period numbers corresponding to each pulse
  std::vector<int> periodNumbers;
};

#endif
