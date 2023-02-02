// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SplittingInterval.h"
#include <string>

using namespace Mantid::Kernel;

namespace Mantid::Kernel {

namespace {
/// static Logger definition
Logger g_log("FilteredTimeSeriesProperty");
} // namespace

/**
 * Constructor
 *  @param name :: The name to assign to the property
 */
template <typename TYPE>
FilteredTimeSeriesProperty<TYPE>::FilteredTimeSeriesProperty(const std::string &name)
    : TimeSeriesProperty<TYPE>(name), m_filter(), m_filterQuickRef(), m_filterApplied() {}

/**
 * Construct with a source time series & a filter property
 * @param seriesProp :: A pointer to a property to filter.
 * @param filterProp :: A boolean series property to filter on
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp,
                                                                 const TimeSeriesProperty<bool> &filterProp)
    : TimeSeriesProperty<HeldType>(*seriesProp),
      m_unfiltered(std::unique_ptr<const TimeSeriesProperty<HeldType>>(seriesProp->clone())), m_filter(),
      m_filterQuickRef(), m_filterApplied() {
  // Now filter us with the filter
  this->filterWith(&filterProp);
}

template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(const std::string &name,
                                                                 const std::vector<Types::Core::DateAndTime> &times,
                                                                 const std::vector<HeldType> &values)
    : TimeSeriesProperty<HeldType>(name, times, values) {}

template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp)
    : TimeSeriesProperty<HeldType>(*seriesProp),
      m_unfiltered(std::unique_ptr<const TimeSeriesProperty<HeldType>>(seriesProp->clone())), m_filter(),
      m_filterQuickRef(), m_filterApplied() {}

/**
 * Construct with a source time series & a filter property
 * @param seriesProp :: A smart pointer to take ownership of pointer to a
 * property to filter.
 * @param filterProp :: A boolean series property to filter on
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(
    std::unique_ptr<const TimeSeriesProperty<HeldType>> seriesProp, const TimeSeriesProperty<bool> &filterProp)
    : TimeSeriesProperty<HeldType>(*seriesProp), m_unfiltered(std::move(seriesProp)), m_filter(), m_filterQuickRef(),
      m_filterApplied() {
  // Now filter us with the filter
  this->filterWith(&filterProp);
}

/**
 * "Virtual" copy constructor
 */
template <typename HeldType> FilteredTimeSeriesProperty<HeldType> *FilteredTimeSeriesProperty<HeldType>::clone() const {
  return new FilteredTimeSeriesProperty<HeldType>(*this);
}

template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(const FilteredTimeSeriesProperty &prop)
    : TimeSeriesProperty<HeldType>(prop.name(), prop.timesAsVector(), prop.valuesAsVector()), m_unfiltered(),
      m_filter(prop.m_filter), m_filterQuickRef(prop.m_filterQuickRef), m_filterApplied(prop.m_filterApplied) {}

/**
 * Get a vector of values taking the filter into account.
 * Values will be excluded if their times lie in a region where the filter is
 * false.
 * @returns :: Vector of included values only
 */
template <typename TYPE> std::vector<TYPE> FilteredTimeSeriesProperty<TYPE>::filteredValuesAsVector() const {
  if (this->m_filter.empty()) {
    return this->valuesAsVector(); // no filtering to do
  }
  if (!this->m_filterApplied) {
    applyFilter();
  }
  this->sortIfNecessary();

  std::vector<TYPE> filteredValues;
  for (const auto &value : this->m_values) {
    if (isTimeFiltered(value.time())) {
      filteredValues.emplace_back(value.value());
    }
  }

  return filteredValues;
}

/**
 * Return the time series's filtered times as a vector<DateAndTime>
 * @return A vector of DateAndTime objects
 */
template <typename HeldType>
std::vector<DateAndTime> FilteredTimeSeriesProperty<HeldType>::filteredTimesAsVector() const {
  if (m_filter.empty()) {
    return this->timesAsVector(); // no filtering to do
  }
  if (!m_filterApplied) {
    applyFilter();
  }
  this->sortIfNecessary();

  std::vector<DateAndTime> out;

  for (const auto &value : this->m_values) {
    if (isTimeFiltered(value.time())) {
      out.emplace_back(value.time());
    }
  }

  return out;
}

template <typename TYPE> double FilteredTimeSeriesProperty<TYPE>::mean() const {
  Mantid::Kernel::Statistics raw_stats =
      Mantid::Kernel::getStatistics(this->filteredValuesAsVector(), StatOptions::Mean);
  return raw_stats.mean;
}

/** Returns n-th valid time interval, in a very inefficient way.
 *
 * Here are some special cases
 *  (1) If empty property, throw runtime_error
 *  (2) If double or more entries, skip!
 *  (3) If n = size of property, use dt from last interval
 *  (4) If n > size of property, return Interval = 0
 *  @param n :: index
 *  @return n-th time interval
 */
template <typename TYPE> TimeInterval FilteredTimeSeriesProperty<TYPE>::nthInterval(int n) const {
  // 0. Throw exception
  if (this->m_values.empty()) {
    const std::string error("nthInterval(): FilteredTimeSeriesProperty '" + this->name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 1. Sort
  this->sortIfNecessary();

  // 2. Calculate time interval

  Kernel::TimeInterval deltaT;

  if (m_filter.empty()) {
    // I. No filter
    if (n >= static_cast<int>(this->m_values.size()) ||
        (n == static_cast<int>(this->m_values.size()) - 1 && this->m_values.size() == 1)) {
      // 1. Out of bound
      ;
    } else if (n == static_cast<int>(this->m_values.size()) - 1) {
      // 2. Last one by making up an end time.
      time_duration d = this->m_values.rbegin()->time() - (this->m_values.rbegin() + 1)->time();
      DateAndTime endTime = this->m_values.rbegin()->time() + d;
      Kernel::TimeInterval dt(this->m_values.rbegin()->time(), endTime);
      deltaT = dt;
    } else {
      // 3. Regular
      DateAndTime startT = this->m_values[static_cast<std::size_t>(n)].time();
      DateAndTime endT = this->m_values[static_cast<std::size_t>(n) + 1].time();
      TimeInterval dt(startT, endT);
      deltaT = dt;
    }
  } else {
    // II. Filter
    // II.0 apply Filter
    this->applyFilter();

    if (static_cast<size_t>(n) > m_filterQuickRef.back().second + 1) {
      // 1. n > size of the allowed region, do nothing to dt
      ;
    } else if (static_cast<size_t>(n) == m_filterQuickRef.back().second + 1) {
      // 2. n = size of the allowed region, duplicate the last one
      auto ind_t1 = static_cast<long>(m_filterQuickRef.back().first);
      long ind_t2 = ind_t1 - 1;
      Types::Core::DateAndTime t1 = (this->m_values.begin() + ind_t1)->time();
      Types::Core::DateAndTime t2 = (this->m_values.begin() + ind_t2)->time();
      time_duration d = t1 - t2;
      Types::Core::DateAndTime t3 = t1 + d;
      Kernel::TimeInterval dt(t1, t3);
      deltaT = dt;
    } else {
      // 3. n < size
      Types::Core::DateAndTime t0;
      Types::Core::DateAndTime tf;

      size_t refindex = this->findNthIndexFromQuickRef(n);
      if (refindex + 3 >= m_filterQuickRef.size())
        throw std::logic_error("nthInterval:  Haven't considered this case.");

      int diff = n - static_cast<int>(m_filterQuickRef[refindex].second);
      if (diff < 0)
        throw std::logic_error("nthInterval:  diff cannot be less than 0.");

      // i) start time
      Types::Core::DateAndTime ftime0 = m_filter[m_filterQuickRef[refindex].first].first;
      size_t iStartIndex = m_filterQuickRef[refindex + 1].first + static_cast<size_t>(diff);
      Types::Core::DateAndTime ltime0 = this->m_values[iStartIndex].time();
      if (iStartIndex == 0 && ftime0 < ltime0) {
        // a) Special case that True-filter time starts before log time
        t0 = ltime0;
      } else if (diff == 0) {
        // b) First entry... usually start from filter time
        t0 = ftime0;
      } else {
        // c) Not the first entry.. usually in the middle of TRUE filter period.
        // use log time
        t0 = ltime0;
      }

      // ii) end time
      size_t iStopIndex = iStartIndex + 1;
      if (iStopIndex >= this->m_values.size()) {
        // a) Last log entry is for the start
        Types::Core::DateAndTime ftimef = m_filter[m_filterQuickRef[refindex + 3].first].first;
        tf = ftimef;
      } else {
        // b) Using the earlier value of next log entry and next filter entry
        Types::Core::DateAndTime ltimef = this->m_values[iStopIndex].time();
        Types::Core::DateAndTime ftimef = m_filter[m_filterQuickRef[refindex + 3].first].first;
        if (ltimef < ftimef)
          tf = ltimef;
        else
          tf = ftimef;
      }

      Kernel::TimeInterval dt(t0, tf);
      deltaT = dt;
    } // END-IF-ELSE Cases
  }

  return deltaT;
}

//-----------------------------------------------------------------------------------------------
/** Returns n-th value of n-th interval in an incredibly inefficient way.
 *  The algorithm is migrated from mthInterval()
 *  @param n :: index
 *  @return Value
 */
template <typename TYPE> TYPE FilteredTimeSeriesProperty<TYPE>::nthValue(int n) const {
  TYPE value;

  // 1. Throw error if property is empty
  if (this->m_values.empty()) {
    const std::string error("nthValue(): TimeSeriesProperty '" + this->name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 2. Sort and apply filter
  this->sortIfNecessary();

  if (m_filter.empty()) {
    // 3. Situation 1:  No filter
    if (static_cast<size_t>(n) < this->m_values.size()) {
      TimeValueUnit<TYPE> entry = this->m_values[static_cast<std::size_t>(n)];
      value = entry.value();
    } else {
      TimeValueUnit<TYPE> entry = this->m_values[static_cast<std::size_t>(this->m_size) - 1];
      value = entry.value();
    }
  } else {
    // 4. Situation 2: There is filter
    this->applyFilter();

    if (static_cast<size_t>(n) > m_filterQuickRef.back().second + 1) {
      // 1. n >= size of the allowed region
      size_t ilog = (m_filterQuickRef.rbegin() + 1)->first;
      value = this->m_values[ilog].value();
    } else {
      // 2. n < size
      Types::Core::DateAndTime t0;
      Types::Core::DateAndTime tf;

      size_t refindex = findNthIndexFromQuickRef(n);
      if (refindex + 3 >= m_filterQuickRef.size()) {
        throw std::logic_error("Not consider out of boundary case here. ");
      }
      size_t ilog =
          m_filterQuickRef[refindex + 1].first + (static_cast<std::size_t>(n) - m_filterQuickRef[refindex].second);
      value = this->m_values[ilog].value();
    } // END-IF-ELSE Cases
  }

  return value;
}

/* Divide the property into  allowed and disallowed time intervals according to
 \a filter.
 * (Repeated time-value pairs (two same time and value entries) mark the start
 of a gap in the values.)
 * If any time-value pair is repeated, it means that this entry is in disallowed
 region.
 * The gap ends and an allowed time interval starts when a single time-value is
 met.
   The disallowed region will be hidden for countSize() and nthInterval()
   Boundary condition
   ?. If filter[0].time > log[0].time, then all log before filter[0] are
 considered TRUE
   2. If filter[-1].time < log[-1].time, then all log after filter[-1] will be
 considered same as filter[-1]

   @param filter :: The filter mask to apply
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::filterWith(const TimeSeriesProperty<bool> *filter) {
  // 1. Clear the current
  m_filter.clear();
  m_filterQuickRef.clear();

  if (filter->size() == 0) {
    // if filter is empty, return
    return;
  }

  // 2. Construct mFilter
  std::vector<Types::Core::DateAndTime> filtertimes = filter->timesAsVector();
  std::vector<bool> filtervalues = filter->valuesAsVector();
  assert(filtertimes.size() == filtervalues.size());
  const size_t nFilterTimes(filtertimes.size());
  m_filter.reserve(nFilterTimes + 1);

  bool lastIsTrue = false;
  auto fend = filtertimes.end();
  auto vit = filtervalues.begin();
  for (auto fit = filtertimes.begin(); fit != fend; ++fit) {
    if (*vit && !lastIsTrue) {
      // Get a true in filter but last recorded value is for false
      m_filter.emplace_back(*fit, true);
      lastIsTrue = true;
    } else if (!(*vit) && lastIsTrue) {
      // Get a False in filter but last recorded value is for TRUE
      m_filter.emplace_back(*fit, false);
      lastIsTrue = false;
    }
    ++vit; // move to next value
  }

  // 2b) Get a clean finish
  if (filtervalues.back()) {
    DateAndTime lastTime, nextLastT;
    if (this->m_values.back().time() > filtertimes.back()) {
      const size_t nvalues(this->m_values.size());
      // Last log time is later than last filter time
      lastTime = this->m_values.back().time();
      if (nvalues > 1 && this->m_values[nvalues - 2].time() > filtertimes.back())
        nextLastT = this->m_values[nvalues - 2].time();
      else
        nextLastT = filtertimes.back();
    } else {
      // Last log time is no later than last filter time
      lastTime = filtertimes.back();
      const size_t nfilterValues(filtervalues.size());
      // If last-but-one filter time is still later than value then previous is
      // this
      // else it is the last value time
      if (nfilterValues > 1 && this->m_values.back().time() > filtertimes[nfilterValues - 2])
        nextLastT = filtertimes[nfilterValues - 2];
      else
        nextLastT = this->m_values.back().time();
    }

    time_duration dtime = lastTime - nextLastT;
    m_filter.emplace_back(lastTime + dtime, false);
  }

  // 3. Reset flag and do filter
  m_filterApplied = false;
  applyFilter();
}

/**
 * Restores the property to the unsorted & unfiltered state
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::clearFilter() {
  m_filter.clear();
  m_filterQuickRef.clear();
}

/**
 * Updates size()
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::countSize() const {
  if (m_filter.empty()) {
    // 1. Not filter
    this->m_size = int(this->m_values.size());
  } else {
    // 2. With Filter
    if (!m_filterApplied) {
      this->applyFilter();
    }
    size_t nvalues = m_filterQuickRef.empty() ? this->m_values.size() : m_filterQuickRef.back().second;
    // The filter logic can end up with the quick ref having a duplicate of the
    // last time and value at the end if the last filter time is past the log
    // time See "If it is out of upper boundary, still record it.  but make the
    // log entry to mP.size()+1" in applyFilter
    // Make the log seem the full size
    if (nvalues == this->m_values.size() + 1) {
      --nvalues;
    }
    this->m_size = static_cast<int>(nvalues);
  }
}

/*
 * Apply filter
 * Requirement: There is no 2 consecutive 'second' values that are same in
 *mFilter
 *
 * It only works with filter starting from TRUE AND having TRUE and FALSE
 *altered
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::applyFilter() const {
  // 1. Check and reset
  if (m_filterApplied)
    return;
  if (m_filter.empty())
    return;

  m_filterQuickRef.clear();

  // 2. Apply filter
  int icurlog = 0;
  for (size_t ift = 0; ift < m_filter.size(); ift++) {
    if (m_filter[ift].second) {
      // a) Filter == True: indicating the start of a quick reference region
      int istart = 0;
      if (icurlog > 0)
        istart = icurlog - 1;

      if (icurlog < static_cast<int>(this->m_values.size()))
        icurlog = this->upperBound(m_filter[ift].first, istart, static_cast<int>(this->m_values.size()) - 1);

      if (icurlog < 0) {
        // i. If it is out of lower boundary, add filter time, add 0 time
        if (!m_filterQuickRef.empty())
          throw std::logic_error("return log index < 0 only occurs with the first log entry");

        m_filterQuickRef.emplace_back(ift, 0);
        m_filterQuickRef.emplace_back(0, 0);

        icurlog = 0;
      } else if (icurlog >= static_cast<int>(this->m_values.size())) {
        // ii.  If it is out of upper boundary, still record it.  but make the
        // log entry to mP.size()+1
        size_t ip = 0;
        if (m_filterQuickRef.size() >= 4)
          ip = m_filterQuickRef.back().second;
        m_filterQuickRef.emplace_back(ift, ip);
        m_filterQuickRef.emplace_back(this->m_values.size() + 1, ip);
      } else {
        // iii. The returned value is in the boundary.
        size_t numintervals = 0;
        if (!m_filterQuickRef.empty()) {
          numintervals = m_filterQuickRef.back().second;
        }
        if (m_filter[ift].first < this->m_values[static_cast<std::size_t>(icurlog)].time()) {
          if (icurlog == 0) {
            throw std::logic_error("In this case, icurlog won't be zero! ");
          }
          icurlog--;
        }
        m_filterQuickRef.emplace_back(ift, numintervals);
        // Note: numintervals inherits from last filter
        m_filterQuickRef.emplace_back(icurlog, numintervals);
      }
    } // Filter value is True
    else if (m_filterQuickRef.size() % 4 == 2) {
      // b) Filter == False: indicating the end of a quick reference region
      int ilastlog = icurlog;

      if (ilastlog < static_cast<int>(this->m_values.size())) {
        // B1: Last TRUE entry is still within log
        icurlog = this->upperBound(m_filter[ift].first, icurlog, static_cast<int>(this->m_values.size()) - 1);

        if (icurlog < 0) {
          // i.   Some false filter is before the first log entry.  The previous
          // filter does not make sense
          if (m_filterQuickRef.size() != 2)
            throw std::logic_error("False filter is before first log entry.  "
                                   "QuickRef size must be 2.");
          m_filterQuickRef.pop_back();
          m_filterQuickRef.clear();
        } else {
          // ii.  Register the end of a valid log
          if (ilastlog < 0)
            throw std::logic_error(" LastLog is not expected to be less than 0");

          int delta_numintervals = icurlog - ilastlog;
          if (delta_numintervals < 0)
            throw std::logic_error("Havn't considered delta numinterval can be less than 0.");

          size_t new_numintervals = m_filterQuickRef.back().second + static_cast<size_t>(delta_numintervals);

          m_filterQuickRef.emplace_back(icurlog, new_numintervals);
          m_filterQuickRef.emplace_back(ift, new_numintervals);
        }
      } else {
        // B2. Last TRUE filter's time is already out side of log.
        size_t new_numintervals = m_filterQuickRef.back().second + 1;
        m_filterQuickRef.emplace_back(icurlog - 1, new_numintervals);
        m_filterQuickRef.emplace_back(ift, new_numintervals);
      }
    } // Filter value is FALSE

  } // ENDFOR

  // 5. Change flag
  m_filterApplied = true;

  // 6. Re-count size
  countSize();
}

/*
 * A new algorithm sto find Nth index.  It is simple and leave a lot work to the
 *callers
 *
 * Return: the index of the quick reference vector
 */
template <typename TYPE> size_t FilteredTimeSeriesProperty<TYPE>::findNthIndexFromQuickRef(int n) const {
  size_t index = 0;

  // 1. Do check
  if (n < 0)
    throw std::invalid_argument("Unable to take into account negative index. ");
  else if (m_filterQuickRef.empty())
    throw std::runtime_error("Quick reference is not established. ");

  // 2. Return...
  if (static_cast<size_t>(n) >= m_filterQuickRef.back().second) {
    // 2A.  Out side of boundary
    index = m_filterQuickRef.size();
  } else {
    // 2B. Inside
    for (size_t i = 0; i < m_filterQuickRef.size(); i += 4) {
      if (static_cast<size_t>(n) >= m_filterQuickRef[i].second &&
          static_cast<size_t>(n) < m_filterQuickRef[i + 3].second) {
        index = i;
        break;
      }
    }
  }

  return index;
}

/**
 * Set the value of the property via a reference to another property.
 * If the value is unacceptable the value is not changed but a string is
 * returned.
 * The value is only accepted if the other property has the same type as this
 * @param right :: A reference to a property.
 */
template <typename TYPE> std::string FilteredTimeSeriesProperty<TYPE>::setValueFromProperty(const Property &right) {
  auto prop = dynamic_cast<const FilteredTimeSeriesProperty<TYPE> *>(&right);
  if (!prop) {
    return "Could not set value: properties have different type.";
  }
  this->m_values = prop->m_values;
  this->m_size = prop->m_size;
  this->m_propSortedFlag = prop->m_propSortedFlag;
  m_filter = prop->m_filter;
  m_filterQuickRef = prop->m_filterQuickRef;
  m_filterApplied = prop->m_filterApplied;
  return "";
}

/**
 * Find out if the given time is included in the filtered data
 * i.e. it does not lie in an excluded region. This function assumes
 * the filter is not empty, it has been applied and the values are
 * sorted by time.
 * @param time :: [input] Time to check
 * @returns :: True if time is in an included region, false if the filter
 * excludes it.
 */
template <typename TYPE>
bool FilteredTimeSeriesProperty<TYPE>::isTimeFiltered(const Types::Core::DateAndTime &time) const {
  // Each time/value pair in the filter defines a point where the region defined
  // after that time is either included/excluded depending on the boolean value.
  // By definition of the filter construction the region before a given filter
  // time must have the opposite value. For times outside the filter region:
  //   1. time < first filter time: inverse of the first filter value
  //   2. time > last filter time: value of the last filter value
  // If time == a filter time then the value is taken to belong to that filter
  // region and not the previous

  // Find first fitler time strictly greater than time
  auto filterEntry = std::lower_bound(m_filter.begin(), m_filter.end(), time,
                                      [](const std::pair<Types::Core::DateAndTime, bool> &filterEntry,
                                         const Types::Core::DateAndTime &t) { return filterEntry.first <= t; });

  if (filterEntry == m_filter.begin()) {
    return !filterEntry->second;
  } else {
    // iterator points to filter greater than time and but we want the previous
    // region
    --filterEntry;
    return filterEntry->second;
  }
}

/**
 * Get a list of the splitting intervals, if filtering is enabled.
 * Otherwise the interval is just first time - last time.
 * @returns :: Vector of splitting intervals
 */
template <typename TYPE>
std::vector<SplittingInterval> FilteredTimeSeriesProperty<TYPE>::getSplittingIntervals() const {
  std::vector<SplittingInterval> intervals;
  // Case where there is no filter
  if (m_filter.empty()) {
    intervals.emplace_back(this->firstTime(), this->lastTime());
    return intervals;
  }

  if (!m_filterApplied) {
    applyFilter();
  }

  // (local reference to use in lambda)
  const auto &localFilter = m_filter;
  /// Count along to find the next time in the filter for which value is 'val'
  const auto findNext = [&localFilter](size_t &index, const bool val) {
    for (; index < localFilter.size(); ++index) {
      const auto &entry = localFilter[index];
      if (entry.second == val) {
        return entry.first;
      }
    }
    return localFilter.back().first;
  };

  // Look through filter to find start/stop pairs
  size_t index = 0;
  while (index < m_filter.size()) {
    DateAndTime start, stop;
    if (index == 0) {
      if (m_filter[0].second) {
        start = m_filter[0].first;
      } else {
        start = this->firstTime();
      }
    } else {
      start = findNext(index, true);
    }
    stop = findNext(index, false);
    if (stop != start) { // avoid empty ranges
      intervals.emplace_back(start, stop);
    }
  }

  return intervals;
}

/**
 * Destructor
 */
template <typename HeldType> FilteredTimeSeriesProperty<HeldType>::~FilteredTimeSeriesProperty() = default;

/**
 * Access the unfiltered log
 * @returns A pointer to the unfiltered property
 */
template <typename HeldType>
const TimeSeriesProperty<HeldType> *FilteredTimeSeriesProperty<HeldType>::unfiltered() const {
  return m_unfiltered.get();
}

/// @cond
// -------------------------- Macro to instantiation concrete types
// --------------------------------
#define INSTANTIATE(TYPE) template class MANTID_KERNEL_DLL FilteredTimeSeriesProperty<TYPE>;

// -------------------------- Concrete instantiation
// -----------------------------------------------
INSTANTIATE(int32_t)
INSTANTIATE(int64_t)
INSTANTIATE(uint32_t)
INSTANTIATE(uint64_t)
INSTANTIATE(float)
INSTANTIATE(double)
INSTANTIATE(std::string)
INSTANTIATE(bool)

///@endcond

} // namespace Mantid::Kernel
