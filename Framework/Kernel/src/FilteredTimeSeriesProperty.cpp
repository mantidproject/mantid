// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeROI.h"
#include <string>

using namespace Mantid::Kernel;
using Mantid::Types::Core::DateAndTime;

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
    : TimeSeriesProperty<TYPE>(name), m_filter(std::make_unique<TimeROI>()), m_filterMap(), m_filterApplied(false) {}

/**
 * Construct with a source time series & a filter property
 * @param seriesProp :: A pointer to a property to filter.
 * @param filterProp :: A boolean series property to filter on
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp,
                                                                 const TimeSeriesProperty<bool> &filterProp)
    : TimeSeriesProperty<HeldType>(*seriesProp), m_filter(std::make_unique<TimeROI>()), m_filterMap(),
      m_filterIntervals(), m_filterApplied(false) {
  // Now filter us with the filter
  this->filterWith(&filterProp);
}

template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(const std::string &name,
                                                                 const std::vector<Types::Core::DateAndTime> &times,
                                                                 const std::vector<HeldType> &values)
    : TimeSeriesProperty<HeldType>(name, times, values), m_filter(std::make_unique<TimeROI>()), m_filterMap(),
      m_filterIntervals(), m_filterApplied(false) {}

template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp)
    : TimeSeriesProperty<HeldType>(*seriesProp), m_filter(std::make_unique<TimeROI>()), m_filterMap(),
      m_filterIntervals(), m_filterApplied(false) {}

/**
 * Construct with a source time series & a filter property
 * @param seriesProp :: A smart pointer to take ownership of pointer to a
 * property to filter.
 * @param filterProp :: A boolean series property to filter on
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(
    std::unique_ptr<const TimeSeriesProperty<HeldType>> seriesProp, const TimeSeriesProperty<bool> &filterProp)
    : TimeSeriesProperty<HeldType>(*seriesProp.get()), m_filter(std::make_unique<TimeROI>()), m_filterMap(),
      m_filterIntervals(), m_filterApplied(false) {
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
    : TimeSeriesProperty<HeldType>(prop.name(), prop.timesAsVector(), prop.valuesAsVector()),
      m_filter(std::make_unique<TimeROI>(*prop.m_filter.get())), m_filterMap(prop.m_filterMap),
      m_filterIntervals(prop.m_filterIntervals), m_filterApplied(prop.m_filterApplied) {}

/**
 * Destructor
 */
template <typename HeldType> FilteredTimeSeriesProperty<HeldType>::~FilteredTimeSeriesProperty() = default;

/**
 * Get a vector of values taking the filter into account.
 * Values will be excluded if their times lie in a region where the filter is
 * false.
 * @returns :: Vector of included values only
 */
template <typename TYPE>
std::vector<TYPE> FilteredTimeSeriesProperty<TYPE>::filteredValuesAsVector(const Kernel::TimeROI *roi) const {
  // if this is not filtered just return the parent version
  if (this->m_filter->useAll()) {
    return TimeSeriesProperty<TYPE>::filteredValuesAsVector(roi); // no filtering to do
  }
  // if the supplied roi is empty use just this one
  const auto internalRoi = this->intersectFilterWithOther(roi); // allocates memory
  const auto result = TimeSeriesProperty<TYPE>::filteredValuesAsVector(internalRoi);
  delete internalRoi;
  return result;
}

template <typename TYPE> std::vector<TYPE> FilteredTimeSeriesProperty<TYPE>::filteredValuesAsVector() const {
  return TimeSeriesProperty<TYPE>::filteredValuesAsVector(this->m_filter.get());
}

/**
 * Return the time series's filtered times as a vector<DateAndTime>
 * @return A vector of DateAndTime objects
 */
template <typename TYPE>
std::vector<DateAndTime> FilteredTimeSeriesProperty<TYPE>::filteredTimesAsVector(const Kernel::TimeROI *roi) const {
  if (m_filter->useAll()) {
    return TimeSeriesProperty<TYPE>::filteredTimesAsVector(roi); // no filtering to do
  }
  const auto internalRoi = this->intersectFilterWithOther(roi);
  return TimeSeriesProperty<TYPE>::filteredTimesAsVector(internalRoi);
}

template <typename TYPE> std::vector<DateAndTime> FilteredTimeSeriesProperty<TYPE>::filteredTimesAsVector() const {
  return TimeSeriesProperty<TYPE>::filteredTimesAsVector(this->m_filter.get());
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
  // Throw exception when there are no values
  if (this->m_values.empty()) {
    const std::string error("nthInterval(): FilteredTimeSeriesProperty '" + this->name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // Calculate time interval
  Kernel::TimeInterval deltaT;
  if (m_filter->useAll()) {
    // No filter uses the parent class implmentation
    deltaT = TimeSeriesProperty<TYPE>::nthInterval(n);
  } else {
    this->applyFilter();
    deltaT = this->m_filterIntervals[std::size_t(n)];
  }

  return deltaT;
}

template <typename TYPE> Types::Core::DateAndTime FilteredTimeSeriesProperty<TYPE>::nthTime(int n) const {
  const auto interval = this->nthInterval(n);
  return interval.start();
}

//-----------------------------------------------------------------------------------------------
/** Returns n-th value of n-th interval in an incredibly inefficient way.
 *  The algorithm is migrated from mthInterval()
 *  @param n :: index
 *  @return Value
 */
template <typename TYPE> TYPE FilteredTimeSeriesProperty<TYPE>::nthValue(int n) const {
  // Throw error if property is empty
  if (this->m_values.empty()) {
    const std::string error("nthValue(): FilteredTimeSeriesProperty '" + this->name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  TYPE value;
  if (m_filter->useAll()) {
    // 3. Situation 1:  No filter
    value = TimeSeriesProperty<TYPE>::nthValue(n);
  } else {
    // 4. Situation 2: There is filter
    this->applyFilter();

    if (m_filterMap.empty()) {
      // this shouldn't happen
      value = this->m_values.back().value();
    } else {
      // going past the end gets the last value
      const size_t n_index = std::min<size_t>(static_cast<size_t>(n), m_filterMap.size() - 1);
      value = this->m_values[m_filterMap[n_index]].value();
    }
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
  if ((!filter) || (filter->size() == 0)) {
    // if filter is empty, clear the current
    this->clearFilter();
  } else {
    this->clearFilterCache();
    if (filter->lastValue() == true) {
      // get the invented end time that has the same duration as the last known duration
      DateAndTime endTime = this->getFakeEndTime();

      // create temporary filter to add this end time to
      TimeSeriesProperty<bool> *filterModified = filter->clone();
      filterModified->addValue(endTime, false);

      m_filter->replaceROI(filterModified);
    } else {
      m_filter->replaceROI(filter);
    }

    applyFilter();
  }
}

template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::filterWith(const TimeROI &filter) {
  if (filter.useAll()) {
    // if filter is empty, clear the current
    this->clearFilter();
  } else {
    this->clearFilterCache();
    m_filter->replaceROI(filter);

    applyFilter();
  }
}

template <typename TYPE> const TimeSeriesProperty<TYPE> *FilteredTimeSeriesProperty<TYPE>::unfiltered() const {
  // copy the values and ignore the filter
  return std::move(new TimeSeriesProperty<TYPE>(this->name(), this->timesAsVector(), this->valuesAsVector()));
}

/**
 * Restores the property to the unsorted & unfiltered state
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::clearFilter() const {
  m_filter->clear();
  this->clearFilterCache();
}

template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::clearFilterCache() const {
  m_filterApplied = false;
  m_filterMap.clear();
  m_filterIntervals.clear();
}

/**
 * Updates size()
 */
template <typename TYPE> void FilteredTimeSeriesProperty<TYPE>::countSize() const {
  if (m_filter->useAll()) {
    // 1. Not filter
    this->m_size = int(this->m_values.size());
  } else {
    // 2. With Filter
    this->applyFilter();
    this->m_size = int(m_filterMap.empty() ? this->m_values.size() : m_filterMap.size());
  }
}

template <typename TYPE> int FilteredTimeSeriesProperty<TYPE>::size() const {
  countSize();
  return this->m_size;
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

  // clear out the previous version
  this->clearFilterCache();

  if (m_filter->useAll())
    return;

  // 2. Apply filter
  this->sortIfNecessary();

  // the index into the m_values array of the time, or -1 (before) or m_values.size() (after)
  std::size_t index_current_log{0};

  for (const auto &splitter : m_filter->toTimeIntervals()) {
    const auto endTime = splitter.stop();

    // check if the splitter starts too early
    if (endTime < this->m_values[index_current_log].time()) {
      continue; // skip to the next splitter
    }

    // cache values to reduce number of method calls
    const auto beginTime = splitter.start();

    // find the first log that should be added
    if (this->m_values.back().time() < beginTime) {
      // skip directly to the end if the filter starts after the last log
      index_current_log = this->m_values.size() - 1;
    } else {
      // search for the right starting point
      while ((this->m_values[index_current_log].time() <= beginTime)) {
        if (index_current_log + 1 > this->m_values.size())
          break;
        index_current_log++;
      }
      // need to back up by one
      if (index_current_log > 0)
        index_current_log--;
      // go backwards more while times are equal to the one being started at
      while (index_current_log > 0 &&
             this->m_values[index_current_log].time() == this->m_values[index_current_log - 1].time()) {
        index_current_log--;
      }
    }

    // add everything up to the end time
    for (; index_current_log < this->m_values.size(); ++index_current_log) {
      if (this->m_values[index_current_log].time() >= endTime)
        break;

      // the current value goes into the filter
      m_filterMap.emplace_back(index_current_log);

      // end time is the end of the filter or when the next value starts
      DateAndTime myEndTime(endTime);
      if (index_current_log + 1 < this->m_values.size())
        myEndTime = std::min(endTime, this->m_values[index_current_log + 1].time());
      // start time is when this value was created or when the filter started
      m_filterIntervals.emplace_back(
          TimeInterval(std::max(beginTime, this->m_values[index_current_log].time()), myEndTime));
    }
    // go back one so the next splitter can add a value
    if (index_current_log > 0)
      index_current_log--;
  }

  // Change flag
  m_filterApplied = true;

  // Re-count size
  countSize();
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
  m_filter = std::unique_ptr<TimeROI>(prop->m_filter.get());
  m_filterMap = prop->m_filterMap;
  m_filterApplied = prop->m_filterApplied;
  return "";
}

/**
 * Combines the currently held filter with the supplied one as an intersection. This assumes caller is responsible for
 * memory.
 */
template <typename TYPE>
Kernel::TimeROI *FilteredTimeSeriesProperty<TYPE>::intersectFilterWithOther(const TimeROI *other) const {
  auto roi = new TimeROI(*m_filter.get());
  if (other && (!other->useAll()))
    roi->update_or_replace_intersection(*other);
  return std::move(roi);
}

template <typename TYPE> const Kernel::TimeROI &FilteredTimeSeriesProperty<TYPE>::getTimeROI() const {
  return *(this->m_filter.get());
}

/**
 * Get a list of the splitting intervals, if filtering is enabled.
 * Otherwise the interval is just first time - last time.
 * @returns :: Vector of splitting intervals
 */
template <typename TYPE> std::vector<TimeInterval> FilteredTimeSeriesProperty<TYPE>::getTimeIntervals() const {
  if (m_filter->useAll()) {
    // Case where there is no filter just use the parent implementation
    return TimeSeriesProperty<TYPE>::getTimeIntervals();
  } else {
    if (!m_filterApplied) {
      applyFilter();
    }

    return m_filter->toTimeIntervals();
  }
}

template <typename HeldType>
double FilteredTimeSeriesProperty<HeldType>::timeAverageValue(const TimeROI *timeRoi) const {
  const TimeROI *roi = intersectFilterWithOther(timeRoi);

  if (size() == 1)
    return static_cast<double>(TimeSeriesProperty<HeldType>::firstValue(*roi));
  else
    return TimeSeriesProperty<HeldType>::timeAverageValue(roi);
}

template <> double FilteredTimeSeriesProperty<std::string>::timeAverageValue(const TimeROI * /*timeRoi*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::timeAverageValue is not implemented for string properties");
}

template <typename HeldType>
bool FilteredTimeSeriesProperty<HeldType>::operator==(const TimeSeriesProperty<HeldType> &right) const {
  const bool time_and_value_compare = TimeSeriesProperty<HeldType>::operator==(right);
  if (!time_and_value_compare) {
    // unfiltered doesn't match
    return false;
  } else {
    // only need to compare the filters
    const auto rhs_ftsp = dynamic_cast<const FilteredTimeSeriesProperty<HeldType> *>(&right);
    if (m_filter->useAll()) {
      if (!rhs_ftsp) {
        // simple compare is fine
        return time_and_value_compare;
      } else {
        // can't compare filters
        return false;
      }
    } else {
      // only need to compare filters
      return m_filter == rhs_ftsp->m_filter;
    }
  }
}

template <typename HeldType> bool FilteredTimeSeriesProperty<HeldType>::operator==(const Property &right) const {
  auto rhs_tsp = dynamic_cast<const TimeSeriesProperty<HeldType> *>(&right);
  if (!rhs_tsp)
    return false;
  return this->operator==(*rhs_tsp);
}

/// @cond
// -------------------------- Macro to instantiation concrete types
// --------------------------------
#define INSTANTIATE(TYPE) template class FilteredTimeSeriesProperty<TYPE>;

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
