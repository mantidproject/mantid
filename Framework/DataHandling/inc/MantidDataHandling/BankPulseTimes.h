// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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

  /// Equals
  bool equals(size_t otherNumPulse, const std::string &otherStartTime);

  /// String describing the start time
  std::string startTime;

  /// Array of the pulse times
  std::vector<Mantid::Types::Core::DateAndTime> pulseTimes;

  /// Vector of period numbers corresponding to each pulse
  std::vector<int> periodNumbers;
};
