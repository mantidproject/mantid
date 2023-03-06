// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +

#include "MantidDataObjects/TimeSplitter.h"
#include "MantidDataObjects/EventList.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeROI.h"

#include <set>

namespace Mantid {
using API::EventType;
using Kernel::TimeROI;
using Types::Core::DateAndTime;

namespace DataObjects {

namespace {
// default value for the output is zero
constexpr int DEFAULT_TARGET{0};

void assertIncreasing(const DateAndTime &start, const DateAndTime &stop) {
  if (start > stop)
    throw std::runtime_error("TODO message");
}

/// static Logger definition
Kernel::Logger g_log("TimeSplitter");

} // namespace

TimeSplitter::TimeSplitter(const DateAndTime &start, const DateAndTime &stop) {
  clearAndReplace(start, stop, DEFAULT_TARGET);
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
    auto index = static_cast<int>(Y[i - 1]);
    if ((index != NO_TARGET) && (valueAtTime(timeStart) != NO_TARGET || valueAtTime(timeEnd) != NO_TARGET)) {
      g_log.warning() << "Values between " << timeStart.second() << "(s) and " << timeEnd.second()
                      << "(s) may be overwritten in conversion to TimeSplitter" << '\n';
    }
    this->addROI(timeStart, timeEnd, index);
  }
}

TimeSplitter::TimeSplitter(const TableWorkspace_sptr &tws, const DateAndTime &offset) {
  if (tws->columnCount() != 3) {
    throw std::runtime_error("Table workspace used for event filtering must have 3 columns.");
  }

  // by design, there should be 3 columns, e.g. "start", "stop", "target", although the exact names are not enforced
  API::Column_sptr col_start = tws->getColumn(0);
  API::Column_sptr col_stop = tws->getColumn(1);
  API::Column_sptr col_target = tws->getColumn(2);

  for (size_t ii = 0; ii < tws->rowCount(); ii++) {
    // by design, the times in the table must be in seconds
    double timeStart_s{col_start->cell<double>(ii)};
    double timeStop_s{col_stop->cell<double>(ii)};
    if (timeStart_s < 0 || timeStop_s < 0) {
      throw std::runtime_error("All times in TableWorkspace must be >= 0 to construct TimeSplitter.");
    }
    Types::Core::DateAndTime timeStart(timeStart_s, 0.0 /*ns*/);
    Types::Core::DateAndTime timeStop(timeStop_s, 0.0 /*ns*/);

    // make the times absolute
    int64_t offset_ns{offset.totalNanoseconds()};
    timeStart += offset_ns;
    timeStop += offset_ns;

    // get the target workspace index
    int target_ws_index = std::stoi(col_target->cell<std::string>(ii));

    // if this row's time interval intersects an interval already in the splitter, no separate ROI will be created
    if ((target_ws_index != NO_TARGET) && (valueAtTime(timeStart) != NO_TARGET || valueAtTime(timeStop) != NO_TARGET)) {
      g_log.warning() << "Workspace row " << ii << " may be overwritten in conversion to TimeSplitter" << '\n';
    }

    addROI(timeStart, timeStop, target_ws_index);
  }
}

TimeSplitter::TimeSplitter(const SplittersWorkspace_sptr &sws) {
  for (size_t ii = 0; ii < sws->rowCount(); ii++) {
    Kernel::SplittingInterval interval = sws->getSplitter(ii);

    // if this row's time interval intersects an interval already in the splitter, no separate ROI will be created
    if (interval.index() != NO_TARGET &&
        (valueAtTime(interval.start()) != NO_TARGET || valueAtTime(interval.stop()) != NO_TARGET)) {
      g_log.warning() << "Workspace row " << ii << " may be overwritten in conversion to TimeSplitter" << '\n';
    }

    addROI(interval.start(), interval.stop(), interval.index());
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

void TimeSplitter::addROI(const DateAndTime &start, const DateAndTime &stop, const int value) {
  assertIncreasing(start, stop);
  if (m_roi_map.empty()) {
    // set the values without checks
    clearAndReplace(start, stop, value);
  } else if ((start <= m_roi_map.begin()->first) && (stop >= m_roi_map.rbegin()->first)) {
    // overwrite existing map
    clearAndReplace(start, stop, value);
  } else if ((stop < m_roi_map.begin()->first) || (start > m_roi_map.rbegin()->first)) {
    // adding to one end or the other
    if (value > NO_TARGET) { // only add non-ignore values
      m_roi_map.insert({start, value});
      m_roi_map.insert({stop, NO_TARGET});
    }
  } else {
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
        m_roi_map.insert({start, value});
      }
    }

    // find the new iterator for where this goes to see if it's the same value
    stopIterator = m_roi_map.lower_bound(stop);
    if ((stopIterator != m_roi_map.end()) && (value == stopIterator->second)) {
      m_roi_map.erase(stopIterator);
    }
    if (value != stopValue) {
      m_roi_map.insert({stop, stopValue});
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
}

/**
 * Find the destination index for an event with a given time.
 * @param time : the time of the event
 * @return : the destination index associated to a
 */
int TimeSplitter::valueAtTime(const DateAndTime &time) const {
  if (m_roi_map.empty())
    return NO_TARGET;
  if (time < m_roi_map.begin()->first)
    return NO_TARGET;

  // this method can be used when the object is in an unusual state and doesn't
  // end with NO_TARGET

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
 * Return a sorted vector of the output workspace indices
 */
std::vector<int> TimeSplitter::outputWorkspaceIndices() const {
  // sets have unique values and are sorted
  std::set<int> outputSet;

  // copy all the non-negative output destination indices
  for (const auto iter : m_roi_map) {
    if (iter.second > NO_TARGET)
      outputSet.insert(iter.second);
  }

  // return a vector
  return std::vector<int>(outputSet.begin(), outputSet.end());
}

/**
 * Returns a Mantid::Kernel::TimeROI for the requested workspace index.
 * This will raise an exception if the workspace index does not exist in the TimeSplitter.
 */
TimeROI TimeSplitter::getTimeROI(const int workspaceIndex) {
  // convert indexes less than NO_TARGET to NO_TARGET
  const int effectiveIndex = std::max<int>(workspaceIndex, NO_TARGET);

  TimeROI output;
  using map_value_type = std::map<DateAndTime, int>::value_type;
  auto indexFinder = [effectiveIndex](const map_value_type &value) { return value.second == effectiveIndex; };
  // find the first place this workspace index exists
  auto iter = std::find_if(m_roi_map.begin(), m_roi_map.end(), indexFinder);
  // add the ROI found then loop until we reach the end
  while (iter != m_roi_map.end()) {
    // add the ROI
    const auto startTime = iter->first;
    iter++;
    // if the next iterator is the end there is nothing to add
    if (iter != m_roi_map.end()) {
      const auto stopTime = iter->first;
      output.addROI(startTime, stopTime);
    }

    // look for the next place the workspace index occurs
    iter = std::find_if(iter, m_roi_map.end(), indexFinder);
  }

  // error check that something is there
  // ignore index being empty is ok
  if ((workspaceIndex >= 0) && (output.empty())) {
    std::stringstream msg;
    msg << "No regions exist for workspace index " << workspaceIndex;
  }

  return output;
}

std::size_t TimeSplitter::numRawValues() const { return m_roi_map.size(); }

// ------------------------------------------------------------------------
// SPLITTING EVENTS METHODS
// ------------------------------------------------------------------------

/**
 * Split a list of events according to Pulse time or Pulse + TOF time
 *
 * Events with masked times are allocated to destination index -1.
 * @param events : list of input events
 * @param partials : resulting partial lists of events
 * @param pulseTof : if True, split according to Pulse + TOF time, otherwise split by Pulse time
 * @param tofCorrect : rescale and shift the TOF values (factor*TOF + shift)
 * @param factor : rescale the TOF values by a dimensionless factor.
 * @param shift : shift the TOF values after rescaling, in units of microseconds.
 * @throws RunTimeError : the event list is of type Mantid::API::EventType::WEIGHTED_NOTIME
 */
void TimeSplitter::splitEventList(const EventList &events, std::map<int, EventList *> partials, bool pulseTof,
                                  bool tofCorrect, double factor, double shift) const {

  if (events.getEventType() == EventType::WEIGHTED_NOTIME)
    throw std::runtime_error("EventList::splitByTime() called on an EventList "
                             "that no longer has time information.");

  // Initialize the detector ID's and event type of the destination event lists
  events.initializePartials(partials);

  if (this->empty())
    return;

  // sort the list in-place
  if (pulseTof)
    // this sorting is preserved under linear transformation tof --> factor*tof+shift with factor>0
    events.sortPulseTimeTOF();
  else
    events.sortPulseTime();

  // fetch the times associated to each event
  std::vector<DateAndTime> times;
  if (pulseTof)
    if (tofCorrect)
      times = events.getPulseTOFTimesAtSample(factor, shift);
    else
      times = events.getPulseTOFTimes();
  else
    times = events.getPulseTimes();

  // split the events
  switch (events.getEventType()) {
  case EventType::TOF:
    this->splitEventVec(times, events.getEvents(), partials);
    break;
  case EventType::WEIGHTED:
    this->splitEventVec(times, events.getWeightedEvents(), partials);
    break;
  default:
    throw std::runtime_error("Unhandled event type");
  }
}

/**
 * Distribute a list of events by comparing a vector of times against the splitter boundaries.
 *
 * Each event in `events` has a corresponding time in `times`, which we use to find a destination index
 * in the TimeSplitter object. The destination index is the key to find the target event list
 * in the partials map.
 *
 * @tparam EVENTTYPE : one of EventType::TOF or EventType::WEIGHTED
 * @param times : times associated to the events, used to find the destination index
 * @param events : list of input times
 * @param partials : target list of partial event lists, associated to the different destination indexes
 * @throws : if the size of times and events are different
 */
template <typename EVENTTYPE>
void TimeSplitter::splitEventVec(const std::vector<DateAndTime> &times, const std::vector<EVENTTYPE> &events,
                                 std::map<int, EventList *> partials) const {
  assert(times.size() == events.size());
  // initialize the iterator over the splitter
  // it assumes the splitter keys (DateAndTime objects) are sorted by increasing time.
  auto itSplitter = m_roi_map.cbegin(); // iterator over the splitter
  DateAndTime stop = itSplitter->first; // first splitter boundary. Discard events with times < stop
  int destination = TimeSplitter::NO_TARGET;

  // is there an EventList mapped to the destination index?
  EventList *partial = nullptr;
  if (partials.find(destination) != partials.cend())
    partial = partials[destination];

  auto itTime = times.cbegin();   // initialize iterator over times
  auto itEvent = events.cbegin(); // initialize iterator over the events

  // iterate over all events. It is assumed events are sorted by either pulse time or tof
  while (itEvent != events.cend()) {
    // Check if we need to advance the splitter and therefore select a different partial event list
    if (*itTime >= stop) {
      // update the partial event list with the destination index of the stopping boundary
      destination = itSplitter->second;
      if (partials.find(destination) == partials.cend())
        partial = nullptr;
      else
        partial = partials[destination];
      // update the stopping boundary
      itSplitter++;
      if (itSplitter == m_roi_map.cend())
        stop = DateAndTime::maximum(); // a.k.a stopping boundary at an "infinite" time
      else
        stop = itSplitter->first;
    }
    if (partial) {
      const EVENTTYPE eventCopy(*itEvent);
      partial->addEventQuickly(eventCopy);
    }
    // advance both the iterator over events and times
    itTime++;
    itEvent++;
  }
}

} // namespace DataObjects
} // namespace Mantid
