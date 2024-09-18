// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataObjects/TimeSplitter.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeROI.h"

namespace Mantid {
using API::EventType;
using Kernel::SplittingInterval;
using Kernel::SplittingIntervalVec;
using Kernel::TimeROI;
using Types::Core::DateAndTime;

namespace DataObjects {

namespace {

void assertIncreasing(const DateAndTime &start, const DateAndTime &stop) {
  if (start > stop)
    throw std::runtime_error("start time found at a later time than stop time");
}

/// static Logger definition
Kernel::Logger g_log("TimeSplitter");

} // namespace

TimeSplitter::TimeSplitter(const TimeSplitter &other) {
  m_roi_map = other.m_roi_map;
  m_name_index_map = other.m_name_index_map;
  m_index_name_map = other.m_index_name_map;
}

TimeSplitter &TimeSplitter::operator=(const TimeSplitter &other) {
  m_roi_map = other.m_roi_map;
  m_name_index_map = other.m_name_index_map;
  m_index_name_map = other.m_index_name_map;

  resetCache();

  return *this;
}

TimeSplitter::TimeSplitter(const DateAndTime &start, const DateAndTime &stop, const int value) {
  clearAndReplace(start, stop, value);
}

/**
 * Note: The amount of X values in input MatrixWorkspace must be 1 larger than the amount of Y values.
 * There are NO undefined split regions here.
 **/
TimeSplitter::TimeSplitter(const Mantid::API::MatrixWorkspace_sptr &ws, const DateAndTime &offset) {
  if (ws->getNumberHistograms() != 1) {
    throw std::runtime_error("MatrixWorkspace can only have 1 histogram when constructing TimeSplitter.");
  }

  const auto X = ws->binEdges(0);
  const auto &Y = ws->y(0);
  if (std::any_of(X.begin(), X.end(), [](double i) { return static_cast<int>(i) < 0; })) {
    throw std::runtime_error("All X values in MatrixWorkspace must be >= 0 to construct TimeSplitter.");
  }
  if (X.size() != Y.size() + 1) {
    throw std::runtime_error(
        "Size of x values must be one more than size of y values to construct TimeSplitter from MatrixWorkspace.");
  }

  int64_t offset_ns{offset.totalNanoseconds()};
  for (size_t i = 1; i < X.size(); i++) {
    auto timeStart = Types::Core::DateAndTime(X[i - 1], 0.0) + offset_ns;
    auto timeEnd = Types::Core::DateAndTime(X[i], 0.0) + offset_ns;
    auto target_index = static_cast<int>(Y[i - 1]);
    if ((target_index != NO_TARGET) && (valueAtTime(timeStart) != NO_TARGET || valueAtTime(timeEnd) != NO_TARGET)) {
      g_log.warning() << "Values between " << timeStart.second() << "(s) and " << timeEnd.second()
                      << "(s) may be overwritten in conversion to TimeSplitter" << '\n';
    }
    this->addROI(timeStart, timeEnd, target_index);
    std::string target_name = std::to_string(target_index);
    m_name_index_map[target_name] = target_index;
    m_index_name_map[target_index] = target_name;
  }
}

TimeSplitter::TimeSplitter(const TableWorkspace_sptr &tws, const DateAndTime &offset) {
  if (tws->columnCount() != 3) {
    throw std::runtime_error("Table workspace used for event filtering must have 3 columns.");
  }

  // by design, there should be 3 columns, e.g. "start", "stop", "target", although the exact names are not enforced
  const std::size_t COL_START{0};
  const std::size_t COL_STOP{1};
  const std::size_t COL_TARGET{2};
  API::Column_sptr col_target = tws->getColumn(COL_TARGET);

  int target_index{NO_TARGET};
  int max_target_index{0};

  g_log.information() << "Creating a time splitter from a table workspace. Total number of rows: " << tws->rowCount()
                      << "\n";

  // can't mix label types except for NO_TARGET
  bool has_target_name_numeric = false;
  bool has_target_name_nonnumeric = false;

  for (size_t row = 0; row < tws->rowCount(); row++) {
    // by design, the times in the table must be in seconds
    const double timeStart_s{tws->cell_cast<double>(row, COL_START)};
    const double timeStop_s{tws->cell_cast<double>(row, COL_STOP)};

    if (timeStart_s < 0 || timeStop_s < 0) {
      throw std::runtime_error("All times in TableWorkspace must be >= 0 to construct TimeSplitter.");
    }

    // create start/stop based from the supplied offset time
    const Types::Core::DateAndTime timeStart = offset + timeStart_s;
    const Types::Core::DateAndTime timeStop = offset + timeStop_s;

    // get the target name; it may or may not represent an integer
    std::string target_name = col_target->cell<std::string>(row);

    const auto targetIter = m_name_index_map.find(target_name);
    if (targetIter == m_name_index_map.end()) {
      // Try converting to integer or create an index
      // If target name represents an integer, that integer automatically becomes the
      // workspace index. If target name is a non-numeric string, we will assign a unique index to it.
      try {
        target_index = std::stoi(target_name);
        m_name_index_map[target_name] = target_index;
        m_index_name_map[target_index] = target_name;
        if (target_index != NO_TARGET)
          has_target_name_numeric = true;
      } catch (std::invalid_argument &) { // a non-integer string
        has_target_name_nonnumeric = true;

        if (m_name_index_map.count(target_name) == 0) {
          target_index = max_target_index;
          m_name_index_map[target_name] = target_index;
          m_index_name_map[target_index] = target_name;
          max_target_index++;
        } else {
          target_index = m_name_index_map[target_name];
          assert(m_index_name_map[target_index] == target_name);
        }
      }
    } else {
      target_index = targetIter->second;
    }

    // if this row's time interval intersects an interval already in the splitter, no separate ROI will be created
    if ((target_index != NO_TARGET) && (valueAtTime(timeStart) != NO_TARGET || valueAtTime(timeStop) != NO_TARGET)) {
      g_log.warning() << "Workspace row " << row << " may be overwritten in conversion to TimeSplitter" << '\n';
    }

    addROI(timeStart, timeStop, target_index);
  }

  // Verify that the input target names are either all numeric or all non-numeric. The exception is a name "-1", i.e. no
  // target specified. That name is ok to mix with non-numeric names.
  if (has_target_name_numeric && has_target_name_nonnumeric)
    throw std::runtime_error("Valid splitter targets cannot be a mix of numeric and non-numeric names.");
}

TimeSplitter::TimeSplitter(const SplittersWorkspace_sptr &sws) {
  for (size_t ii = 0; ii < sws->rowCount(); ii++) {
    Kernel::SplittingInterval interval = sws->getSplitter(ii);

    // if this row's time interval intersects an interval already in the splitter, no separate ROI will be created
    if (interval.index() != NO_TARGET &&
        (valueAtTime(interval.start()) != NO_TARGET || valueAtTime(interval.stop()) != NO_TARGET)) {
      g_log.warning() << "Workspace row " << ii << " may be overwritten in conversion to TimeSplitter" << '\n';
    }

    int target_index = interval.index();
    addROI(interval.start(), interval.stop(), target_index);
    std::string target_name = std::to_string(target_index);
    m_name_index_map[target_name] = target_index;
    m_index_name_map[target_index] = target_name;
  }
}

/// Print the (destination index | DateAndTime boundary) pairs of this splitter.
std::string TimeSplitter::debugPrint() const {
  std::stringstream msg;
  for (const auto &iter : m_roi_map)
    msg << iter.second << "|" << iter.first << "\n";
  return msg.str();
}

const std::map<DateAndTime, int> &TimeSplitter::getSplittersMap() const { return m_roi_map; }

// Get the target name from the target index.
std::string TimeSplitter::getWorkspaceIndexName(const int workspaceIndex, const int numericalShift) {
  if (m_index_name_map.count(workspaceIndex) == 0) {
    std::stringstream msg;
    msg << "Invalid target index " << workspaceIndex << " when calling TimeSplitter::getWorkspaceIndexName";
    throw std::runtime_error(msg.str());
  }

  std::string target_name = m_index_name_map[workspaceIndex];

  // If numericalShift is not zero, the "_index" suffix of the name will be shifted.
  // This is needed to support FilterEvents property OutputWorkspaceIndexedFrom1.
  if (numericalShift != 0) {
    // If this TimeSplitter was built from a TableWorkspace, targets could be non-numeric, in which case a numeric
    // shift wouldn't make sense.
    int target_index;
    try {
      target_index = std::stoi(target_name);
    } catch (std::invalid_argument &) // a non-integer string
    {
      throw std::runtime_error(
          "FilterEvents property \"OutputWorkspaceIndexedFrom1\" is not compatible with non-numeric targets.");
    }

    assert(target_index == m_name_index_map[target_name]);
    std::stringstream s;
    s << target_index + numericalShift;
    return s.str();
  }

  return target_name;
}

void TimeSplitter::addROI(const DateAndTime &start, const DateAndTime &stop, const int value) {
  resetCache();

  // If start time == stop time, the map will be corrupted.
  if (start == stop)
    return;
  assertIncreasing(start, stop);
  if (m_roi_map.empty()) {
    // set the values without checks
    clearAndReplace(start, stop, value);
    return;
  }
  const DateAndTime &firstTime = m_roi_map.begin()->first;
  const DateAndTime &lastTime = m_roi_map.rbegin()->first;
  if ((start <= firstTime) && (stop >= lastTime)) {
    // the new ROI covers the whole of the existing TimeSplitter, thus replace it
    clearAndReplace(start, stop, value);
  } else if ((stop < firstTime) || (start > lastTime)) {
    // the new ROI lies outside the whole of the existing TimeSplitter, thus just add it
    if (value > NO_TARGET) { // only add non-ignore values
      m_roi_map.emplace(start, value);
      m_roi_map.emplace(stop, NO_TARGET);
    }
  } else if (start == lastTime) { // the new ROI starts at the end of the existing TimeSplitter
    if (value > NO_TARGET) {      // only add non-ignore values
      m_roi_map.rbegin()->second = value;
      m_roi_map.emplace(stop, NO_TARGET);
    }
  } else if (stop == firstTime) { // the new ROI ends at the start of the existing TimeSplitter
    if (value > NO_TARGET) {      // only add non-ignore values
      m_roi_map.emplace(start, value);
    }
  } else { // the new ROI either overlaps or is inside the existing TimeSplitter
    // do the interesting version
    g_log.debug() << "addROI(" << start << ", " << stop << ", " << value << ")\n";

    // cache what the final value will be
    const int stopValue = this->valueAtTime(stop);

    // find if there are values to erase

    // the starting point is greater than or equal to the "start" supplied
    auto startIterator = m_roi_map.lower_bound(start);
    if ((startIterator->first != start) && (startIterator != m_roi_map.begin()))
      startIterator--; // move to the one before

    // the end is one past the "stop"
    auto stopIterator = m_roi_map.upper_bound(stop);
    if ((stopIterator != m_roi_map.end()) && (stopValue == NO_TARGET))
      stopIterator++; // move to the one after

    const bool atStart = (startIterator == m_roi_map.begin());

    // remove the elements that are being replaced [inclusive, exclusive)
    m_roi_map.erase(startIterator, stopIterator);

    // put in the new elements
    if ((value > NO_TARGET) || (!atStart)) {
      if (value != this->valueAtTime(start)) {
        m_roi_map.emplace(start, value);
      }
    }

    // find the new iterator for where this goes to see if it's the same value
    stopIterator = m_roi_map.lower_bound(stop);
    if ((stopIterator != m_roi_map.end()) && (value == stopIterator->second)) {
      m_roi_map.erase(stopIterator);
    }
    if (value != stopValue) {
      m_roi_map.emplace(stop, stopValue);
    }

    // verify this ends with NO_TARGET
    if (m_roi_map.rbegin()->second != NO_TARGET) {
      throw std::runtime_error("Something went wrong in TimeSplitter::addROI");
    }
  }
}

/// Check if the TimeSplitter is empty
bool TimeSplitter::empty() const { return m_roi_map.empty(); }

void TimeSplitter::clearAndReplace(const DateAndTime &start, const DateAndTime &stop, const int value) {
  m_roi_map.clear();
  if (value >= 0) {
    m_roi_map.insert({start, value});
    m_roi_map.insert({stop, NO_TARGET});
  }
  resetCache();
}

// Invalidate all cached "views" on the class implementation, i.e. m_roi_map.
// Every time m_roi_map changes, the cached "views" need to be invalidated.
void TimeSplitter::resetCache() {
  resetCachedPartialTimeROIs();
  resetCachedSplittingIntervals();
}

// Invalidate cached partial TimeROIs, so that the next call to getTimeROI() would trigger their rebuild.
void TimeSplitter::resetCachedPartialTimeROIs() const {
  if (m_validCachedPartialTimeROIs) {
    m_cachedPartialTimeROIs.clear();
    m_validCachedPartialTimeROIs = false;
  }
}

// Invalidate cached splitting intervals, so that the next call to getSplittingIntervals() would trigger their rebuild
void TimeSplitter::resetCachedSplittingIntervals() const {
  if (m_validCachedSplittingIntervals_All || m_validCachedSplittingIntervals_WithValidTargets) {
    m_cachedSplittingIntervals.clear();
    m_validCachedSplittingIntervals_All = false;
    m_validCachedSplittingIntervals_WithValidTargets = false;
  }
}

// Rebuild and mark as valid a cached map of partial TimeROIs. The getTimeROI() method will then use that map to quickly
// look up and return a TimeROI.
void TimeSplitter::rebuildCachedPartialTimeROIs() const {
  resetCachedPartialTimeROIs();

  if (empty())
    return;

  // Loop over m_roi_map and build all partial TimeROIs
  auto it = m_roi_map.cbegin();
  for (; it != std::prev(m_roi_map.cend()); it++) {
    DateAndTime intervalStart = it->first;
    DateAndTime intervalStop = std::next(it)->first;
    int target = it->second;

    if (m_cachedPartialTimeROIs.count(target) > 0) {
      m_cachedPartialTimeROIs[target].appendROIFast(intervalStart, intervalStop);
    } else {
      m_cachedPartialTimeROIs[target] = TimeROI();
      m_cachedPartialTimeROIs[target].appendROIFast(intervalStart, intervalStop);
    }
  }

  if (it->second != NO_TARGET) {
    std::ostringstream err;
    err << "Open-ended time interval is invalid in event filtering: " << it->first << " - ?,"
        << " target index: " << it->second << std::endl;
    throw std::runtime_error(err.str());
  }

  m_validCachedPartialTimeROIs = true;
}

// Rebuild and mark as valid a cached vector of splitting intervals. The getSplittingIntervals() method will then return
// that cached vector without building it.
void TimeSplitter::rebuildCachedSplittingIntervals(const bool includeNoTarget) const {
  resetCachedSplittingIntervals();

  if (empty())
    return;

  auto it = m_roi_map.cbegin();
  const auto itEnd = m_roi_map.cend();
  while (std::next(it) != itEnd) {
    // invoke constructor SplittingInterval(DateAndTime &start, DateAndTime &stop, int index)
    if (includeNoTarget || it->second != NO_TARGET)
      m_cachedSplittingIntervals.emplace_back(it->first, std::next(it)->first, it->second);
    std::advance(it, 1);
  }

  if (it->second != NO_TARGET) {
    std::ostringstream err;
    err << "Open-ended time interval is invalid in event filtering: " << it->first << " - ?,"
        << " target index: " << it->second << std::endl;
    throw std::runtime_error(err.str());
  }

  m_validCachedSplittingIntervals_All = includeNoTarget;
  m_validCachedSplittingIntervals_WithValidTargets = !includeNoTarget;
}

/**
 * Find the destination index for an event with a given time.
 * @param time : event time
 * @return : the destination index associated to the event time
 */
int TimeSplitter::valueAtTime(const DateAndTime &time) const {
  // empty means exclude everything
  if (m_roi_map.empty())
    return NO_TARGET;

  // before the beginning is excluded
  if (time < m_roi_map.cbegin()->first)
    return NO_TARGET;

  // this method can be used when the object is in an unusual state and doesn't
  // end with NO_TARGET
  if (time >= m_roi_map.crbegin()->first)
    return m_roi_map.crbegin()->second;

  // find location that is greater than or equal to the requested time and give
  // back previous value
  auto location = m_roi_map.lower_bound(time);
  if (location->first == time) {
    // found the time in the map
    return location->second;
  } else if (location == m_roi_map.begin()) {
    // iterator is greater than the first value in the map b/c equal is already
    // handled asked for a time outside of the map
    return NO_TARGET;
  } else {
    // go to the value before
    location--;
    return location->second;
  }
}

/**
 * Return a set of the output workspace indices
 */
std::set<int> TimeSplitter::outputWorkspaceIndices() const {
  // sets have unique values and are sorted
  std::set<int> outputSet;

  // copy all the non-negative output destination indices
  for (const auto &iter : m_roi_map) {
    if (iter.second > NO_TARGET)
      outputSet.insert(iter.second);
  }

  return outputSet;
}

/**
 * Returns a TimeROI for the requested workspace index.
 * If the workspace index does not exist in the TimeSplitter, the returned TimeROI will be empty,
 * meaning any time datapoint will pass through the given ROI filter.
 * For efficiency, the first call to this method, after TimeSplitter is built, will trigger calculation of _all_
 * TimeROIs. The following calls to this method will look up and return the corresponding TimeROI without spending time
 * on building it.
 * @param workspaceIndex : target, a.k.a. partial workspace, or destination, index, for which to get a TimeROI.
 * @return : TimeROI for the input target
 */
const TimeROI &TimeSplitter::getTimeROI(const int workspaceIndex) const {
  std::lock_guard<std::mutex> lock(m_mutex);

  if (!m_validCachedPartialTimeROIs) {
    rebuildCachedPartialTimeROIs();
  }
  // convert indexes less than NO_TARGET to NO_TARGET
  const int effectiveIndex = std::max<int>(workspaceIndex, NO_TARGET);

  if (m_cachedPartialTimeROIs.count(effectiveIndex) == 0)
    return TimeROI::USE_ALL;

  return m_cachedPartialTimeROIs[effectiveIndex];
}

/**
 * Returns a vector of splitting intervals corresponding to the m_roi_map.
 * For efficiency, the actual vector calculation should be done only on demand ("lazy") and only when the current vector
 * is invalid.
 * @param includeNoTarget : if true/false, the calculated vector will/will not include intervals with NO_TARGET.
 * @return : a reference to the vector of splitting intervals
 */
const Kernel::SplittingIntervalVec &TimeSplitter::getSplittingIntervals(const bool includeNoTarget) const {
  std::lock_guard<std::mutex> lock(m_mutex);

  if ((includeNoTarget && !m_validCachedSplittingIntervals_All) ||
      (!includeNoTarget && !m_validCachedSplittingIntervals_WithValidTargets)) {
    rebuildCachedSplittingIntervals(includeNoTarget);
  }

  return m_cachedSplittingIntervals;
}

std::size_t TimeSplitter::numRawValues() const { return m_roi_map.size(); }
const std::map<std::string, int> &TimeSplitter::getNameTargetMap() const { return m_name_index_map; }
const std::map<int, std::string> &TimeSplitter::getTargetNameMap() const { return m_index_name_map; }

// ------------------------------------------------------------------------
// SPLITTING EVENTS METHODS
// ------------------------------------------------------------------------

/**
 * Split a list of events according to Pulse time or Pulse + TOF time.
 * This does not clear out the partial EventLists.
 *
 * Events with masked times are allocated to destination index -1.
 * @param events : list of input events
 * @param partials : resulting partial lists of events
 * @param pulseTof : if True, split according to Pulse + TOF time, otherwise split by Pulse time
 * @param tofCorrect : rescale and shift the TOF values (factor*TOF + shift)
 * @param factor : rescale the TOF values by a dimensionless factor.
 * @param shift : shift the TOF values after rescaling, in units of microseconds.
 * @throws invalid_argument : the event list is of type Mantid::API::EventType::WEIGHTED_NOTIME
 */
void TimeSplitter::splitEventList(const EventList &events, std::map<int, EventList *> &partials, const bool pulseTof,
                                  const bool tofCorrect, const double factor, const double shift) const {

  if (events.getEventType() == EventType::WEIGHTED_NOTIME)
    throw std::invalid_argument("EventList::splitEventList() called on an EventList "
                                "that no longer has time information.");

  if (this->empty())
    return;

  // sort the input EventList in-place
  const EventSortType sortOrder = pulseTof ? EventSortType::PULSETIMETOF_SORT
                                           : EventSortType::PULSETIME_SORT; // this will be used to set order on outputs
  if (pulseTof) {
    // this sorting is preserved under linear transformation tof --> factor*tof+shift with factor>0
    events.sortPulseTimeTOF();
  } else {
    events.sortPulseTime();
  }

  // split the events
  switch (events.getEventType()) {
  case EventType::TOF:
    this->splitEventVec(events.getEvents(), partials, pulseTof, tofCorrect, factor, shift);
    break;
  case EventType::WEIGHTED:
    this->splitEventVec(events.getWeightedEvents(), partials, pulseTof, tofCorrect, factor, shift);
    break;
  default:
    throw std::runtime_error("Unhandled event type");
  }

  // set the sort order on the EventLists since we know the sorting already
  for (auto &partial : partials) {
    if (!partial.second->empty())
      partial.second->setSortOrder(sortOrder);
  }
}

/**
 * Distribute a list of events by comparing event times against the splitter boundaries.
 *
 * For each event in `events` we calculate the event time using a timeCalc function. The function definition
 * depends on the input flags (pulseTof, tofCorrect) and input parameters (factor, shift).
 * The calculated time is then used to find a destination index for the event in the TimeSplitter object.
 * The destination index, in turn, is the key to find the target event list in the partials map.
 *
 * @tparam EventType : one of EventType::TOF or EventType::WEIGHTED
 * @param events : list of input events
 * @param partials : target list of partial event lists associated with different destination indexes
 * @param pulseTof : if true, split according to Pulse + TOF time, otherwise split by Pulse time
 * @param tofCorrect : rescale and shift the TOF values (factor*TOF + shift)
 * @param factor : rescale the TOF values by a dimensionless factor.
 * @param shift : shift the TOF values after rescaling, in units of microseconds.
 */
template <typename EventType>

void TimeSplitter::splitEventVec(const std::vector<EventType> &events, std::map<int, EventList *> &partials,
                                 const bool pulseTof, const bool tofCorrect, const double factor,
                                 const double shift) const {
  // determine the right function for getting the "pulse time" for the event
  std::function<const DateAndTime(const EventType &)> timeCalc;
  if (pulseTof) {
    if (tofCorrect) {
      timeCalc = [factor, shift](const EventType &event) { return event.pulseTOFTimeAtSample(factor, shift); };
    } else {
      timeCalc = [](const EventType &event) { return event.pulseTOFTime(); };
    }
  } else {
    timeCalc = [](const EventType &event) { return event.pulseTime(); };
  }

  // do the actual event splitting
  this->splitEventVec(timeCalc, events, partials);
}

template <typename EventType>
void TimeSplitter::splitEventVec(const std::function<const DateAndTime(const EventType &)> &timeCalc,
                                 const std::vector<EventType> &events, std::map<int, EventList *> &partials) const {
  // get a reference of the splitters as a vector
  const auto &splittersVec = getSplittingIntervals(true);

  // initialize the iterator over the splitter
  auto itSplitter = splittersVec.cbegin();
  const auto itSplitterEnd = splittersVec.cend();

  // initialize iterator over the events
  auto itEvent = events.cbegin();
  const auto itEventEnd = events.cend();

  // copy all events before first splitter to NO_TARGET
  auto partial = partials.find(TimeSplitter::NO_TARGET);
  {
    const auto stop = itSplitter->start();
    const bool shouldAppend = (partial != partials.end());
    while (itEvent != itEventEnd && timeCalc(*itEvent) < stop) {
      if (shouldAppend)
        partial->second->addEventQuickly(*itEvent); // emplaces a copy of *itEvent in partial
      // advance event iterator
      itEvent++;
    }
  }

  // iterate over all events. For each event try finding its destination event list, a.k.a. partial.
  // If the partial is found, append the event to it. It is assumed events are sorted by (possibly corrected) time
  while (itEvent != itEventEnd && itSplitter != itSplitterEnd) {
    // Check if we need to advance the splitter and therefore select a different partial event list
    const auto eventTime = timeCalc(*itEvent);
    // advance to the new stopping boundary, and update the destination index as we go
    if (eventTime > itSplitter->stop()) {
      // first try next splitter
      itSplitter++;
      if (itSplitter == itSplitterEnd)
        break;

      // then try lower_bound
      if (itSplitter->stop() < eventTime) {
        itSplitter =
            std::lower_bound(itSplitter, itSplitterEnd, eventTime,
                             [](const auto &splitter, const auto &eventTime) { return splitter.start() < eventTime; });
        // may need to go back one
        if ((std::prev(itSplitterEnd))->stop() > eventTime)
          itSplitter = std::prev(itSplitter);
      }
    }
    if (itSplitter == itSplitterEnd)
      break;

    // determine the new stop time and destination
    const int destination = itSplitter->index();
    const auto stop = itSplitter->stop();

    // find the new partial to add to
    partial = partials.find(destination);

    // loop over events up to the end of the roi
    const bool shouldAppend = (partial != partials.end());
    while (itEvent != itEventEnd && timeCalc(*itEvent) < stop) {
      if (shouldAppend)
        partial->second->addEventQuickly(*itEvent); // emplaces a copy of *itEvent in partial
      // advance event iterator
      itEvent++;
    }

    // increment to the next interval
    itSplitter++;
  }

  // copy all events after last splitter to NO_TARGET
  if (itEvent != itEventEnd) {
    partial = partials.find(TimeSplitter::NO_TARGET);
    if (partial != partials.end()) {
      for (; itEvent != itEventEnd; ++itEvent) {
        partial->second->addEventQuickly(*itEvent); // emplaces a copy of *itEvent in partial
      }
    }
  }
}

} // namespace DataObjects
} // namespace Mantid
