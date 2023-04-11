// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/MatrixWorkspace.h"
#include "MantidDataObjects/DllConfig.h"
#include "MantidDataObjects/SplittersWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidKernel/DateAndTime.h"

#include <set>

namespace Mantid {

using Types::Core::DateAndTime;

namespace Kernel {
class TimeROI; // Forward declaration
}

namespace DataObjects {

class EventList; // Forward declaration

class MANTID_DATAOBJECTS_DLL TimeSplitter {

public:
  static constexpr int NO_TARGET{-1}; // no target (a.k.a. destination) workspace for filtered out events
  TimeSplitter() = default;
  TimeSplitter(const DateAndTime &start, const DateAndTime &stop, const int value = DEFAULT_TARGET);
  TimeSplitter(const Mantid::API::MatrixWorkspace_sptr &ws, const DateAndTime &offset = DateAndTime(0, 0));
  TimeSplitter(const TableWorkspace_sptr &tws, const DateAndTime &offset = DateAndTime(0, 0));
  TimeSplitter(const SplittersWorkspace_sptr &sws);
  const std::map<DateAndTime, int> &getSplittersMap() const;
  std::string getWorkspaceIndexName(const int workspaceIndex, const int numericalShift = 0);
  /// Find the destination index for an event with a given time
  int valueAtTime(const DateAndTime &time) const;
  void addROI(const DateAndTime &start, const DateAndTime &stop, const int value);
  /// Check if the TimeSplitter is empty
  bool empty() const;
  std::set<int> outputWorkspaceIndices() const;
  Kernel::TimeROI getTimeROI(const int workspaceIndex);
  /// Cast to to vector of SplittingInterval objects
  Kernel::SplittingIntervalVec toSplitters(const bool includeNoTarget = true) const;
  /// this is to aid in testing and not intended for use elsewhere
  std::size_t numRawValues() const;
  /// Split a list of events according to Pulse time or Pulse + TOF time
  void splitEventList(const EventList &events, std::map<int, EventList *> partials, bool pulseTof = false,
                      bool tofCorrect = false, double factor = 1.0, double shift = 0.0) const;
  /// Print the (destination index | DateAndTime boundary) pairs of this splitter.
  std::string debugPrint() const;

private:
  static constexpr int DEFAULT_TARGET{0};
  void clearAndReplace(const DateAndTime &start, const DateAndTime &stop, const int value);
  /// Distribute a list of events by comparing a vector of times against the splitter boundaries.
  template <typename EVENTTYPE>
  void splitEventVec(const std::vector<DateAndTime> &times, const std::vector<EVENTTYPE> &events,
                     std::map<int, EventList *> partials) const;
  std::map<DateAndTime, int> m_roi_map;
  // These 2 maps are complementary to each other
  std::map<std::string, int> m_name_index_map;
  std::map<int, std::string> m_index_name_map;
};
} // namespace DataObjects
} // namespace Mantid