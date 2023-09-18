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

static double elapsed_time_case_1{0.0};
static double elapsed_time_case_2{0.0};
static double elapsed_time_case_4{0.0};
static double elapsed_time_case_5{0.0};
// static double elapsed_time_case_10{0.0};

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
  API::Column_sptr col_start = tws->getColumn(0);
  API::Column_sptr col_stop = tws->getColumn(1);
  API::Column_sptr col_target = tws->getColumn(2);

  int target_index{NO_TARGET};
  int max_target_index{0};
  size_t noninteger_target_names_count{0};
  size_t notarget_names_count{0}; // count "-1" targets

  size_t number_of_zero_width_intervals(0);

  std::cout << "Creating a time splitter from a table workspace. Total number of rows: " << tws->rowCount()
            << std::endl;

  for (size_t ii = 0; ii < tws->rowCount(); ii++) {
    // by design, the times in the table must be in seconds
    double timeStart_s{col_start->toDouble(ii)};
    double timeStop_s{col_stop->toDouble(ii)};

    if (timeStart_s == timeStop_s) {
      number_of_zero_width_intervals++;
      continue;
    }

    if (timeStart_s < 0 || timeStop_s < 0) {
      throw std::runtime_error("All times in TableWorkspace must be >= 0 to construct TimeSplitter.");
    }
    Types::Core::DateAndTime timeStart(timeStart_s, 0.0 /*ns*/);
    Types::Core::DateAndTime timeStop(timeStop_s, 0.0 /*ns*/);

    // make the times absolute
    int64_t offset_ns{offset.totalNanoseconds()};
    timeStart += offset_ns;
    timeStop += offset_ns;

    // get the target name; it may or may not represent an integer
    std::string target_name = col_target->cell<std::string>(ii);
    // get the target workspace index. If target name represents an integer, that integer automatically becomes the
    // workspace index. If target name is a non-numeric string, we will assign a unique index to it.
    try {
      target_index = std::stoi(target_name);
      m_name_index_map[target_name] = target_index;
      m_index_name_map[target_index] = target_name;
      if (target_index == NO_TARGET)
        notarget_names_count++;
    } catch (std::invalid_argument &) // a non-integer string
    {
      noninteger_target_names_count++;

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

    // if this row's time interval intersects an interval already in the splitter, no separate ROI will be created
    if ((target_index != NO_TARGET) && (valueAtTime(timeStart) != NO_TARGET || valueAtTime(timeStop) != NO_TARGET)) {
      g_log.warning() << "Workspace row " << ii << " may be overwritten in conversion to TimeSplitter" << '\n';
    }

    addROI(timeStart, timeStop, target_index);
  }

  std::cout << "NUMBER OF ZERO-WIDTH SPLITTER INTERVALS: " << number_of_zero_width_intervals << std::endl;

  // Verify that the input target names are either all numeric or all non-numeric. The exception is a name "-1", i.e. no
  // target specified. That name is ok to mix with non-numeric names.
  // if (noninteger_target_names_count != 0 && noninteger_target_names_count != tws->rowCount() - notarget_names_count)
  // {
  if (noninteger_target_names_count != 0 &&
      noninteger_target_names_count != tws->rowCount() - notarget_names_count - number_of_zero_width_intervals) {
    throw std::runtime_error("Valid splitter targets cannot be a mix of numeric and non-numeric names.");
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

  // If numericalShift > 0, the caller will get back a shifted index.
  // This is needed for supporting FilterEvents property OutputWorkspaceIndexedFrom1.
  assert(numericalShift >= 0);
  if (numericalShift > 0) {
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
  resetTargetTimeVectors();

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
      m_roi_map.insert({start, value});
      m_roi_map.insert({stop, NO_TARGET});
    }
  } else if (start == lastTime) { // the new ROI starts at the end of the existing TimeSplitter
    if (value > NO_TARGET) {      // only add non-ignore values
      m_roi_map.rbegin()->second = value;
      m_roi_map.insert({stop, NO_TARGET});
    }
  } else if (stop == firstTime) { // the new ROI ends at the start of the existing TimeSplitter
    if (value > NO_TARGET) {      // only add non-ignore values
      m_roi_map.insert({start, value});
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
  resetTargetTimeVectors();
}

// Target time vectors must be reset every time the ROI map is modified.
// They must be rebuilt on demand, after the ROI map is settled.
void TimeSplitter::resetTargetTimeVectors() {
  if (validTargetTimeVectors) {
    m_targetTimeVectors.clear();
    validTargetTimeVectors = false;
  }
}

void TimeSplitter::addTimeIntervalToTargetVector(const DateAndTime &intervalStart, const DateAndTime &intervalStop,
                                                 const int target) {
  if (m_targetTimeVectors.count(target) > 0) {
    if (m_targetTimeVectors[target].back() == intervalStart)
      m_targetTimeVectors[target].back() = intervalStop;
    else {
      m_targetTimeVectors[target].push_back(intervalStart);
      m_targetTimeVectors[target].push_back(intervalStop);
    }
  } else {
    m_targetTimeVectors[target] = std::vector<DateAndTime>();
    m_targetTimeVectors[target].push_back(intervalStart);
    m_targetTimeVectors[target].push_back(intervalStop);
  }
}

void TimeSplitter::rebuildTargetTimeVectors() {
  auto start = std::chrono::system_clock::now();
  resetTargetTimeVectors();
  for (auto it = m_roi_map.begin(); it != std::prev(m_roi_map.end()); it++) {
    addTimeIntervalToTargetVector(it->first, std::next(it)->first, it->second);
  }
  validTargetTimeVectors = true;
  auto stop = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = stop - start;
  elapsed_time_case_5 += elapsed_seconds.count();
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
 * Returns a Mantid::Kernel::TimeROI for the requested workspace index.
 * This will raise an exception if the workspace index does not exist in the TimeSplitter.
 */
TimeROI TimeSplitter::getTimeROI(const int workspaceIndex) {

  std::cout << "NUMBER OF ITEMS IN ROI MAP: " << m_roi_map.size() << std::endl;

#if 1
  if (!validTargetTimeVectors) {
    std::cout << "***REBUILDING PARTIAL TIME SPLITTER VECTORS***"
              << "\n";
    rebuildTargetTimeVectors();
  }

  // convert indexes less than NO_TARGET to NO_TARGET
  const int effectiveIndex = std::max<int>(workspaceIndex, NO_TARGET);
  return TimeROI(m_targetTimeVectors[effectiveIndex]);
#endif

#if 0
  auto start = std::chrono::system_clock::now();
  TimeROI output;
  // convert indexes less than NO_TARGET to NO_TARGET
  const int effectiveIndex = std::max<int>(workspaceIndex, NO_TARGET);
  using map_value_type = std::map<DateAndTime, int>::value_type;
  auto indexFinder = [effectiveIndex](const map_value_type &value) { return value.second == effectiveIndex; };
  auto iter = m_roi_map.begin();
  int n = 0;
  while ((iter = std::find_if(iter, m_roi_map.end(), indexFinder)) != m_roi_map.end()) {
    const auto startTime = iter->first;
    if (++iter != m_roi_map.end()) { // if the next iterator is the end, there is nothing to add
      const auto stopTime = iter->first;
      output.addROI(startTime, stopTime);
    }
    if (effectiveIndex == 0 && n % 10000 == 0)
      std::cout << "TimeSplitter::getTimeROI iteration: " << n << std::endl;
    n++;
  }

  // error check that something is there
  // ignore index being empty is ok
  if (effectiveIndex >= 0 && output.useAll()) { // useAll==true means the ROI is empty
    std::stringstream msg;
    msg << "No regions exist for workspace index " << workspaceIndex;
  }

  auto stop = std::chrono::system_clock::now();
  std::chrono::duration<double> elapsed_seconds = stop - start;
  elapsed_time_case_10 += elapsed_seconds.count();

  return output;
#endif
}

/**
 * Cast to a vector of SplittingInterval objects
 */
SplittingIntervalVec TimeSplitter::toSplitters(const bool includeNoTarget) const {
  std::vector<SplittingInterval> output;
  if (this->empty())
    return output;
  auto startIt = m_roi_map.begin();
  while (std::next(startIt) != m_roi_map.end()) {
    /// invoke constructor SplittingInterval(DateAndTime &start, DateAndTime &stop, int index)
    if (includeNoTarget || startIt->second != NO_TARGET)
      output.push_back({startIt->first, std::next(startIt)->first, startIt->second});
    std::advance(startIt, 1);
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
 * @throws invalid_argument : the event list is of type Mantid::API::EventType::WEIGHTED_NOTIME
 */
void TimeSplitter::splitEventList(const EventList &events, std::map<int, EventList *> &partials, bool pulseTof,
                                  bool tofCorrect, double factor, double shift) const {

  if (events.getEventType() == EventType::WEIGHTED_NOTIME)
    throw std::invalid_argument("EventList::splitEventList() called on an EventList "
                                "that no longer has time information.");

  // Initialize the detector ID's and event type of the destination event lists
  events.initializePartials(partials);

  if (this->empty())
    return;

  // sort the list in-place
  EventSortType sortOrder; // this will be used to set order on outputs
  if (pulseTof) {
    // this sorting is preserved under linear transformation tof --> factor*tof+shift with factor>0
    auto start = std::chrono::system_clock::now();
    events.sortPulseTimeTOF();
    sortOrder = EventSortType::PULSETIMETOF_SORT;
    auto stop = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = stop - start;
    elapsed_time_case_1 += elapsed_seconds.count();
  } else {
    auto start = std::chrono::system_clock::now();
    events.sortPulseTime();
    sortOrder = EventSortType::PULSETIME_SORT;
    auto stop = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = stop - start;
    elapsed_time_case_2 += elapsed_seconds.count();
  }

  {
    // split the events
    auto start = std::chrono::system_clock::now();
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
    auto stop = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = stop - start;
    elapsed_time_case_4 += elapsed_seconds.count();
  }

  // set the sort order on the EventLists since we know the sorting already
  for (auto &partial : partials) {
    if (!partial.second->empty())
      partial.second->setSortOrder(sortOrder);
  }
}

/**
 * Distribute a list of events by comparing a vector of times against the splitter boundaries.
 *
 * Each event in `events` has a corresponding time in `times`, which we use to find a destination index
 * in the TimeSplitter object. The destination index is the key to find the target event list
 * in the partials map.
 *
 * @tparam EventType : one of EventType::TOF or EventType::WEIGHTED
 * @param events : list of input events
 * @param partials : target list of partial event lists, associated to the different destination indexes
 * @param pulseTof : if True, split according to Pulse + TOF time, otherwise split by Pulse time
 * @param tofCorrect : rescale and shift the TOF values (factor*TOF + shift)
 * @param factor : rescale the TOF values by a dimensionless factor.
 * @param shift : shift the TOF values after rescaling, in units of microseconds.
 */
template <typename EventType>
void TimeSplitter::splitEventVec(const std::vector<EventType> &events, std::map<int, EventList *> &partials,
                                 bool pulseTof, bool tofCorrect, double factor, double shift) const {
  // determine the right functnio for getting the "pulse time" for the event
  std::function<DateAndTime(const EventType &)> timeCalc;
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
void TimeSplitter::splitEventVec(const std::function<DateAndTime(const EventType &)> &timeCalc,
                                 const std::vector<EventType> &events, std::map<int, EventList *> &partials) const {
  // initialize the iterator over the splitter
  // it assumes the splitter keys (DateAndTime objects) are sorted by increasing time.
  auto itSplitter = m_roi_map.cbegin(); // iterator over the splitter
  DateAndTime stop = itSplitter->first; // first splitter boundary. Discard events with times < stop
  int destination = TimeSplitter::NO_TARGET;

  // is there an EventList mapped to the destination index?
  auto partial = partials.find(destination);

  auto itEvent = events.cbegin(); // initialize iterator over the events
  const auto itEventEnd = events.cend();

  const auto m_roi_map_cend = m_roi_map.cend();
  // iterate over all events. For each event try finding its destination event list, a.k.a. partial.
  // If the partial is found, append the event to it. It is assumed events are sorted by either pulse time or tof
  while (itEvent != itEventEnd) {
    // Check if we need to advance the splitter and therefore select a different partial event list
    const auto eventTime = timeCalc(*itEvent);
    if (eventTime >= stop) {
      // advance to the new stopping boundary, and find the new destination index
      while (itSplitter != m_roi_map_cend && eventTime >= itSplitter->first) {
        itSplitter++;
      }
      // determine the new stop time
      if (itSplitter == m_roi_map_cend) {
        stop = DateAndTime::maximum(); // a.k.a stopping boundary at an "infinite" time
      } else {
        stop = itSplitter->first;
      }
      // determine the new destination
      if (itSplitter == m_roi_map.cbegin()) {
        destination = -1;
      } else { // get the value from the previous iterator
        itSplitter--;
        destination = itSplitter->second;
        itSplitter++;
      }
      // find the new partial to add to
      partial = partials.find(destination);
    }

    // loop over events up to the end of the roi
    const bool shouldAppend = (partial != partials.end());
    while (timeCalc(*itEvent) < stop && itEvent != itEventEnd) {
      if (shouldAppend)
        partial->second->addEventQuickly(*itEvent); // emplaces a copy of *itEvent in partial
      // advance event iterator
      itEvent++;
    }
  }
}

double TimeSplitter::getTime1() { return elapsed_time_case_1; }
double TimeSplitter::getTime2() { return elapsed_time_case_2; }
double TimeSplitter::getTime4() { return elapsed_time_case_4; }
double TimeSplitter::getTime5() { return elapsed_time_case_5; }
// double TimeSplitter::getTime10() { return elapsed_time_case_10; }
} // namespace DataObjects
} // namespace Mantid
