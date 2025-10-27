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
  TimeSplitter(const TimeSplitter &other);
  TimeSplitter &operator=(const TimeSplitter &other);

  TimeSplitter(const DateAndTime &start, const DateAndTime &stop, const int value = DEFAULT_TARGET);
  TimeSplitter(const Mantid::API::MatrixWorkspace_sptr &ws, const DateAndTime &offset = DateAndTime::GPS_EPOCH);
  TimeSplitter(const TableWorkspace_sptr &tws, const DateAndTime &offset = DateAndTime::GPS_EPOCH);
  TimeSplitter(const SplittersWorkspace_sptr &sws);

  const std::map<DateAndTime, int> &getSplittersMap() const;
  std::string getWorkspaceIndexName(const int workspaceIndex, const int numericalShift = 0) const;
  /// Find the destination index for an event with a given time
  int valueAtTime(const DateAndTime &time) const;
  void addROI(const DateAndTime &start, const DateAndTime &stop, const int value);
  /// Check if the TimeSplitter is empty
  bool empty() const;
  std::set<int> outputWorkspaceIndices() const;
  const Kernel::TimeROI &getTimeROI(const int workspaceIndex) const;
  const Kernel::SplittingIntervalVec &getSplittingIntervals(const bool includeNoTarget = true) const;

  /// these methods are to aid in testing and not intended for use elsewhere
  std::size_t numRawValues() const;
  const std::map<std::string, int> &getNameTargetMap() const;
  const std::map<int, std::string> &getTargetNameMap() const;

  /// Split a list of events according to Pulse time or Pulse + TOF time
  void splitEventList(const EventList &events, std::map<int, EventList *> &partials, const bool pulseTof = false,
                      const bool tofCorrect = false, const double factor = 1.0, const double shift = 0.0) const;
  /// Given a list of times, calculate the corresponding indices in the TimeSplitter
  std::vector<std::pair<int, std::pair<size_t, size_t>>>
  calculate_target_indices(const std::vector<DateAndTime> &times) const;

  /// Print the (destination index | DateAndTime boundary) pairs of this splitter.
  std::string debugPrint() const;

private:
  static constexpr int DEFAULT_TARGET{0};
  void clearAndReplace(const DateAndTime &start, const DateAndTime &stop, const int value);
  /// Distribute a list of events by comparing a vector of times against the splitter boundaries.
  template <typename EventType>
  void splitEventVec(const std::vector<EventType> &events, std::map<int, EventList *> &partials, const bool pulseTof,
                     const bool tofCorrect, const double factor, const double shift) const;
  template <typename EventType>
  void splitEventVec(const std::function<const DateAndTime(const EventType &)> &timeCalc,
                     const std::vector<EventType> &events, std::map<int, EventList *> &partials) const;

  void resetCache();
  void resetCachedPartialTimeROIs() const;
  void resetCachedSplittingIntervals() const;

  void rebuildCachedPartialTimeROIs() const;
  void rebuildCachedSplittingIntervals(const bool includeNoTarget = true) const;

private:
  std::map<DateAndTime, int> m_roi_map;
  // These 2 maps are complementary to each other
  std::map<std::string, int> m_name_index_map;
  std::map<int, std::string> m_index_name_map;

  mutable std::map<int, Kernel::TimeROI> m_cachedPartialTimeROIs;
  mutable Kernel::SplittingIntervalVec m_cachedSplittingIntervals;

  mutable bool m_validCachedPartialTimeROIs{false};
  mutable bool m_validCachedSplittingIntervals_All{false};
  mutable bool m_validCachedSplittingIntervals_WithValidTargets{false};

  mutable std::mutex m_mutex;
};
} // namespace DataObjects
} // namespace Mantid
