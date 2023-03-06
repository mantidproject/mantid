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

namespace Mantid {

using Types::Core::DateAndTime;

namespace Kernel {
class TimeROI; // Forward declaration
}
using Kernel::TimeROI;

namespace DataObjects {

class EventList; // Forward declaration

class MANTID_DATAOBJECTS_DLL TimeSplitter {

public:
  static constexpr int NO_TARGET{-1}; // no target (a.k.a. destination) workspace for filtered out events
  TimeSplitter() = default;
  TimeSplitter(const DateAndTime &start, const DateAndTime &stop);
  TimeSplitter(const Mantid::API::MatrixWorkspace_sptr &ws, const DateAndTime &offset = DateAndTime(0, 0));
  TimeSplitter(const TableWorkspace_sptr &tws, const DateAndTime &offset = DateAndTime(0, 0));
  TimeSplitter(const SplittersWorkspace_sptr &sws);
  const std::map<DateAndTime, int> &getSplittersMap() const;
  /// Find the destination index for an event with a given time
  int valueAtTime(const DateAndTime &time) const;
  void addROI(const DateAndTime &start, const DateAndTime &stop, const int value);
  /// Check if the TimeSplitter is empty
  bool empty() const;
  std::vector<int> outputWorkspaceIndices() const;
  TimeROI getTimeROI(const int workspaceIndex);
  /// this is to aid in testing and not intended for use elsewhere
  std::size_t numRawValues() const;
  /// Split a list of events according to Pulse time or Pulse + TOF time
  void splitEventList(const EventList &events, std::map<int, EventList *> partials, bool pulseTof = false,
                      bool tofCorrect = false, double factor = 1.0, double shift = 0.0) const;

private:
  void clearAndReplace(const DateAndTime &start, const DateAndTime &stop, const int value);
  /// In-place sort a list of input events either by Pulse time or by Pulse+TOF time
  std::vector<int64_t> sortEventList(const EventList &events, bool pulseTof = false, bool tofCorrect = false,
                                     double factor = 1.0, double shift = 0.0) const;
  /// Distribute a list of events by comparing a vector of times against the splitter boundaries.
  template <typename EVENTTYPE>
  void splitEventVec(const std::vector<int64_t> &times, const std::vector<EVENTTYPE> &events,
                     std::map<int, EventList *> partials) const;
  /// Print the (destination index | DateAndTime boundary) pairs of this splitter.
  std::string debugPrint() const;
  std::map<DateAndTime, int> m_roi_map;
};
} // namespace DataObjects
} // namespace Mantid