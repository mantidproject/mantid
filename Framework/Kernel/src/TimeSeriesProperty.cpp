// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/TimeSplitter.h"

#include <json/value.h>
#include <nexus/NeXusFile.hpp>

#include <boost/regex.hpp>
#include <numeric>

namespace Mantid {
using namespace Types::Core;
namespace Kernel {
namespace {
/// static Logger definition
Logger g_log("TimeSeriesProperty");
} // namespace

/**
 * Constructor
 *  @param name :: The name to assign to the property
 */
template <typename TYPE>
TimeSeriesProperty<TYPE>::TimeSeriesProperty(const std::string &name)
    : Property(name, typeid(std::vector<TimeValueUnit<TYPE>>)), m_values(),
      m_size(), m_propSortedFlag(), m_filterApplied() {}

/**
 * Constructor
 * @param name :: The name to assign to the property
 * @param times :: A vector of DateAndTime objects
 * @param values :: A vector of TYPE
 */
template <typename TYPE>
TimeSeriesProperty<TYPE>::TimeSeriesProperty(
    const std::string &name, const std::vector<Types::Core::DateAndTime> &times,
    const std::vector<TYPE> &values)
    : TimeSeriesProperty(name) {
  addValues(times, values);
}

/// Virtual destructor
template <typename TYPE> TimeSeriesProperty<TYPE>::~TimeSeriesProperty() {}

/**
 * "Virtual" copy constructor
 */
template <typename TYPE>
TimeSeriesProperty<TYPE> *TimeSeriesProperty<TYPE>::clone() const {
  return new TimeSeriesProperty<TYPE>(*this);
}

/**
 * "Virutal copy constructor with a time shift
 * @param timeShift :: a time shift in seconds
 */
template <typename TYPE>
Property *
TimeSeriesProperty<TYPE>::cloneWithTimeShift(const double timeShift) const {
  auto timeSeriesProperty = this->clone();
  auto values = timeSeriesProperty->valuesAsVector();
  auto times = timeSeriesProperty->timesAsVector();
  // Shift the time
  for (auto it = times.begin(); it != times.end(); ++it) {
    // There is a known issue which can cause cloneWithTimeShift to be called
    // with a large (~9e+9 s) shift. Actual shifting is capped to be ~4.6e+19
    // seconds in DateAndTime::operator+=
    (*it) += timeShift;
  }
  timeSeriesProperty->clear();
  timeSeriesProperty->addValues(times, values);
  return timeSeriesProperty;
}

/** Return time series property, containing time derivative of current property.
 * The property itself and the returned time derivative become sorted by time
 * and the derivative is calculated in seconds^-1. (e.g. dValue/dT where
 * dT=t2-t1 is time difference in seconds for subsequent time readings and
 * dValue=Val1-Val2 is difference in subsequent values)
 *
 */
template <typename TYPE>
std::unique_ptr<TimeSeriesProperty<double>>
TimeSeriesProperty<TYPE>::getDerivative() const {

  if (this->m_values.size() < 2) {
    throw std::runtime_error("Derivative is not defined for a time-series "
                             "property with less then two values");
  }

  this->sortIfNecessary();
  auto it = this->m_values.begin();
  int64_t t0 = it->time().totalNanoseconds();
  TYPE v0 = it->value();

  it++;
  auto timeSeriesDeriv = std::make_unique<TimeSeriesProperty<double>>(
      this->name() + "_derivative");
  timeSeriesDeriv->reserve(this->m_values.size() - 1);
  for (; it != m_values.end(); it++) {
    TYPE v1 = it->value();
    int64_t t1 = it->time().totalNanoseconds();
    if (t1 != t0) {
      double deriv = 1.e+9 * (double(v1 - v0) / double(t1 - t0));
      int64_t tm = static_cast<int64_t>((t1 + t0) / 2);
      timeSeriesDeriv->addValue(Types::Core::DateAndTime(tm), deriv);
    }
    t0 = t1;
    v0 = v1;
  }
  return timeSeriesDeriv;
}
/** time series derivative specialization for string type */
template <>
std::unique_ptr<TimeSeriesProperty<double>>
TimeSeriesProperty<std::string>::getDerivative() const {
  throw std::runtime_error(
      "Time series property derivative is not defined for strings");
}

/**
 * Return the memory used by the property, in bytes
 * */
template <typename TYPE>
size_t TimeSeriesProperty<TYPE>::getMemorySize() const {
  // Rough estimate
  return m_values.size() * (sizeof(TYPE) + sizeof(DateAndTime));
}

/**
 * Just returns the property (*this) unless overridden
 *  @param rhs a property that is merged in some descendent classes
 *  @return a property with the value
 */
template <typename TYPE>
TimeSeriesProperty<TYPE> &TimeSeriesProperty<TYPE>::merge(Property *rhs) {
  return operator+=(rhs);
}

/**
 * Add the value of another property
 * @param right the property to add
 * @return the sum
 */
template <typename TYPE>
TimeSeriesProperty<TYPE> &TimeSeriesProperty<TYPE>::
operator+=(Property const *right) {
  TimeSeriesProperty<TYPE> const *rhs =
      dynamic_cast<TimeSeriesProperty<TYPE> const *>(right);

  if (rhs) {
    if (this->operator!=(*rhs)) {
      m_values.insert(m_values.end(), rhs->m_values.begin(),
                      rhs->m_values.end());
      m_propSortedFlag = TimeSeriesSortStatus::TSUNKNOWN;
    } else {
      // Do nothing if appending yourself to yourself. The net result would be
      // the same anyway
      ;
    }

    // Count the REAL size.
    m_size = static_cast<int>(m_values.size());

  } else
    g_log.warning() << "TimeSeriesProperty " << this->name()
                    << " could not be added to another property of the same "
                       "name but incompatible type.\n";

  return *this;
}

/**
 * Deep comparison.
 * @param right The other property to compare to.
 * @return true if the are equal.
 */
template <typename TYPE>
bool TimeSeriesProperty<TYPE>::
operator==(const TimeSeriesProperty<TYPE> &right) const {
  sortIfNecessary();

  if (this->name() != right.name()) // should this be done?
  {
    return false;
  }

  if (this->m_size != right.m_size) {
    return false;
  }

  if (this->realSize() != right.realSize()) {
    return false;
  } else {
    const std::vector<DateAndTime> lhsTimes = this->timesAsVector();
    const std::vector<DateAndTime> rhsTimes = right.timesAsVector();
    if (!std::equal(lhsTimes.begin(), lhsTimes.end(), rhsTimes.begin())) {
      return false;
    }

    const std::vector<TYPE> lhsValues = this->valuesAsVector();
    const std::vector<TYPE> rhsValues = right.valuesAsVector();
    if (!std::equal(lhsValues.begin(), lhsValues.end(), rhsValues.begin())) {
      return false;
    }
  }

  return true;
}

/**
 * Deep comparison.
 * @param right The other property to compare to.
 * @return true if the are equal.
 */
template <typename TYPE>
bool TimeSeriesProperty<TYPE>::operator==(const Property &right) const {
  auto rhs_tsp = dynamic_cast<const TimeSeriesProperty<TYPE> *>(&right);
  if (!rhs_tsp)
    return false;
  return this->operator==(*rhs_tsp);
}

/**
 * Deep comparison (not equal).
 * @param right The other property to compare to.
 * @return true if the are not equal.
 */
template <typename TYPE>
bool TimeSeriesProperty<TYPE>::
operator!=(const TimeSeriesProperty<TYPE> &right) const {
  return !(*this == right);
}

/**
 * Deep comparison (not equal).
 * @param right The other property to compare to.
 * @return true if the are not equal.
 */
template <typename TYPE>
bool TimeSeriesProperty<TYPE>::operator!=(const Property &right) const {
  return !(*this == right);
}

/**
 * Set name of the property
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::setName(const std::string &name) {
  m_name = name;
}

/** Filter out a run by time. Takes out any TimeSeriesProperty log entries
 *outside of the given
 *  absolute time range.
 *  Be noticed that this operation is not reversible.
 *
 *  Use case 1: if start time of the filter fstart is in between t1 and t2 of
 *the TimeSeriesProperty,
 *              then, the new start time is fstart and the value of the log is
 *the log value @ t1
 *
 *  Use case 2: if the start time of the filter in on t1 or before log start
 *time t0, then
 *              the new start time is t1/t0/filter start time.
 *
 * EXCEPTION: If there is only one entry in the list, it is considered to mean
 * "constant" so the value is kept even if the time is outside the range.
 *
 * @param start :: Absolute start time. Any log entries at times >= to this time
 *are kept.
 * @param stop  :: Absolute stop time. Any log entries at times < than this time
 *are kept.
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::filterByTime(
    const Types::Core::DateAndTime &start,
    const Types::Core::DateAndTime &stop) {
  // 0. Sort
  sortIfNecessary();

  // 1. Do nothing for single (constant) value
  if (m_values.size() <= 1)
    return;

  typename std::vector<TimeValueUnit<TYPE>>::iterator iterhead, iterend;

  // 2. Determine index for start and remove  Note erase is [...)
  int istart = this->findIndex(start);
  if (istart >= 0 && static_cast<size_t>(istart) < m_values.size()) {
    // "start time" is behind time-series's starting time
    iterhead = m_values.begin() + istart;

    // False - The filter time is on the mark.  Erase [begin(),  istart)
    // True - The filter time is larger than T[istart]. Erase[begin(), istart)
    // ...
    //       filter start(time) and move istart to filter startime
    bool useprefiltertime = !(m_values[istart].time() == start);

    // Remove the series
    m_values.erase(m_values.begin(), iterhead);

    if (useprefiltertime) {
      m_values[0].setTime(start);
    }
  } else {
    // "start time" is before/after time-series's starting time: do nothing
    ;
  }

  // 3. Determine index for end and remove  Note erase is [...)
  int iend = this->findIndex(stop);
  if (static_cast<size_t>(iend) < m_values.size()) {
    if (m_values[iend].time() == stop) {
      // Filter stop is on a log.  Delete that log
      iterend = m_values.begin() + iend;
    } else {
      // Filter stop is behind iend. Keep iend
      iterend = m_values.begin() + iend + 1;
    }
    // Delete from [iend to mp.end)
    m_values.erase(iterend, m_values.end());
  }

  // 4. Make size consistent
  m_size = static_cast<int>(m_values.size());
}

/**
 * Filter by a range of times. If current property has a single value it remains
 * unaffected
 * @param splittervec :: A list of intervals to split filter on
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::filterByTimes(
    const std::vector<SplittingInterval> &splittervec) {
  // 1. Sort
  sortIfNecessary();

  // 2. Return for single value
  if (m_values.size() <= 1) {
    return;
  }

  // 3. Prepare a copy
  std::vector<TimeValueUnit<TYPE>> mp_copy;

  g_log.debug() << "DB541  mp_copy Size = " << mp_copy.size()
                << "  Original MP Size = " << m_values.size() << "\n";

  // 4. Create new
  for (const auto &splitter : splittervec) {
    Types::Core::DateAndTime t_start = splitter.start();
    Types::Core::DateAndTime t_stop = splitter.stop();

    int tstartindex = findIndex(t_start);
    if (tstartindex < 0) {
      // The splitter is not well defined, and use the first
      tstartindex = 0;
    } else if (tstartindex >= int(m_values.size())) {
      // The splitter is not well defined, adn use the last
      tstartindex = int(m_values.size()) - 1;
    }

    int tstopindex = findIndex(t_stop);

    if (tstopindex < 0) {
      tstopindex = 0;
    } else if (tstopindex >= int(m_values.size())) {
      tstopindex = int(m_values.size()) - 1;
    } else {
      if (t_stop == m_values[size_t(tstopindex)].time() &&
          size_t(tstopindex) > 0) {
        tstopindex--;
      }
    }

    /* Check */
    if (tstartindex < 0 || tstopindex >= int(m_values.size())) {
      g_log.warning() << "Memory Leak In SplitbyTime!\n";
    }

    if (tstartindex == tstopindex) {
      TimeValueUnit<TYPE> temp(t_start, m_values[tstartindex].value());
      mp_copy.push_back(temp);
    } else {
      mp_copy.emplace_back(t_start, m_values[tstartindex].value());
      for (size_t im = size_t(tstartindex + 1); im <= size_t(tstopindex);
           ++im) {
        mp_copy.emplace_back(m_values[im].time(), m_values[im].value());
      }
    }
  } // ENDFOR

  g_log.debug() << "DB530  Filtered Log Size = " << mp_copy.size()
                << "  Original Log Size = " << m_values.size() << "\n";

  // 5. Clear
  m_values.clear();
  m_values = mp_copy;
  mp_copy.clear();

  m_size = static_cast<int>(m_values.size());
}

/**
 * Split this time series property by time intervals to multiple time series
 * property according to number of distinct splitters' indexes, such as 0 and 1
 *
 * NOTE: If the input TSP has a single value, it is assumed to be a constant
 *  and so is not split, but simply copied to all outputs.
 *
 * @param splitter :: a TimeSplitterType object containing the list of intervals
 *                     and destinations.
 * @param outputs  :: A vector of output TimeSeriesProperty pointers of the same
 *                    type.
 * @param isPeriodic :: whether the log (this TSP) is periodic. For example
 *                    proton-charge is periodic log.
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::splitByTime(
    std::vector<SplittingInterval> &splitter, std::vector<Property *> outputs,
    bool isPeriodic) const {
  // 0. Sort if necessary
  sortIfNecessary();

  if (outputs.empty())
    return;

  std::vector<TimeSeriesProperty<TYPE> *> outputs_tsp;

  size_t numOutputs = outputs.size();
  // 1. Clear the outputs before you start
  for (size_t i = 0; i < numOutputs; i++) {
    TimeSeriesProperty<TYPE> *myOutput =
        dynamic_cast<TimeSeriesProperty<TYPE> *>(outputs[i]);
    if (myOutput) {
      outputs_tsp.push_back(myOutput);
      if (this->m_values.size() == 1) {
        // Special case for TSP with a single entry = just copy.
        myOutput->m_values = this->m_values;
        myOutput->m_size = 1;
      } else {
        myOutput->m_values.clear();
        myOutput->m_size = 0;
      }
    } else {
      outputs_tsp.push_back(nullptr);
    }
  }

  // 2. Special case for TSP with a single entry = just copy.
  if (this->m_values.size() == 1)
    return;

  // 3. We will be iterating through all the entries in the the map/vector
  size_t i_property = 0;

  //    And at the same time, iterate through the splitter
  auto itspl = splitter.begin();

  size_t counter = 0;
  g_log.debug() << "[DB] Number of time series entries = " << m_values.size()
                << ", Number of splitters = " << splitter.size() << "\n";
  while (itspl != splitter.end() && i_property < m_values.size()) {
    // Get the splitting interval times and destination
    DateAndTime start = itspl->start();
    DateAndTime stop = itspl->stop();

    int output_index = itspl->index();
    // output workspace index is out of range. go to the next splitter
    if (output_index < 0 || output_index >= static_cast<int>(numOutputs))
      continue;

    TimeSeriesProperty<TYPE> *myOutput = outputs_tsp[output_index];
    // skip if the input property is of wrong type
    if (!myOutput) {
      ++itspl;
      ++counter;
      continue;
    }

    // Skip the events before the start of the time
    while (i_property < m_values.size() && m_values[i_property].time() < start)
      ++i_property;

    if (i_property == m_values.size()) {
      // i_property is out of the range. Then use the last entry
      myOutput->addValue(m_values[i_property - 1].time(),
                         m_values[i_property - 1].value());

      ++itspl;
      ++counter;
      break;
    }

    // The current entry is within an interval. Record them until out
    if (m_values[i_property].time() > start && i_property > 0 && !isPeriodic) {
      // Record the previous oneif this property is not exactly on start time
      //   and this entry is not recorded
      size_t i_prev = i_property - 1;
      if (myOutput->size() == 0 ||
          m_values[i_prev].time() != myOutput->lastTime())
        myOutput->addValue(m_values[i_prev].time(), m_values[i_prev].value());
    }

    // Loop through all the entries until out.
    while (i_property < m_values.size() && m_values[i_property].time() < stop) {

      // Copy the log out to the output
      myOutput->addValue(m_values[i_property].time(),
                         m_values[i_property].value());
      ++i_property;
    }

    // Go to the next interval
    ++itspl;
    ++counter;
    // But if we reached the end, then we are done.
    if (itspl == splitter.end())
      break;

    // No need to keep looping through the filter if we are out of events
    if (i_property == this->m_values.size())
      break;

  } // Looping through entries in the splitter vector

  // Make sure all entries have the correct size recorded in m_size.
  for (std::size_t i = 0; i < numOutputs; i++) {
    TimeSeriesProperty<TYPE> *myOutput =
        dynamic_cast<TimeSeriesProperty<TYPE> *>(outputs[i]);
    if (myOutput) {
      myOutput->m_size = myOutput->realSize();
    }
  }
}

/// Split this TimeSeriresProperty by a vector of time with N entries,
/// and by the target workspace index defined by target_vec
/// Requirements:
/// vector outputs must be defined before this method is called
template <typename TYPE>
void TimeSeriesProperty<TYPE>::splitByTimeVector(
    std::vector<DateAndTime> &splitter_time_vec, std::vector<int> &target_vec,
    std::vector<TimeSeriesProperty *> outputs) {

  // check target vector to make it a set
  std::set<int> target_set;
  for (auto target : target_vec)
    target_set.insert(target);

  // check inputs
  if (splitter_time_vec.size() != target_vec.size() + 1) {
    std::stringstream errss;
    errss << "Try to split TSP " << this->m_name
          << ": Input time vector's size " << splitter_time_vec.size()
          << " does not match (one more larger than) taget "
             "workspace index vector's size "
          << target_vec.size() << "\n";
    throw std::runtime_error(errss.str());
  }
  // return if the output vector TimeSeriesProperties is not defined
  if (outputs.empty())
    return;

  // sort if necessary
  sortIfNecessary();

  // work on m_values, m_size, and m_time
  std::vector<Types::Core::DateAndTime> tsp_time_vec = this->timesAsVector();

  // go over both filter time vector and time series property time vector
  size_t index_splitter = 0;
  size_t index_tsp_time = 0;

  // tsp_time is start time of time series property
  DateAndTime tsp_time = tsp_time_vec[index_tsp_time];
  DateAndTime split_start_time = splitter_time_vec[index_splitter];
  DateAndTime split_stop_time = splitter_time_vec[index_splitter + 1];

  // move splitter index such that the first entry of TSP is before the stop
  // time of a splitter
  bool continue_search = true;
  bool no_entry_in_range = false;

  std::vector<DateAndTime>::iterator splitter_iter;
  splitter_iter = std::lower_bound(splitter_time_vec.begin(),
                                   splitter_time_vec.end(), tsp_time);
  if (splitter_iter == splitter_time_vec.begin()) {
    // do nothing as the first TimeSeriesProperty entry's time is before any
    // splitters
    ;
  } else if (splitter_iter == splitter_time_vec.end()) {
    // already search to the last splitter which is still earlier than first TSP
    // entry
    no_entry_in_range = true;
  } else {
    // calculate the splitter's index (now we check the stop time)
    index_splitter = splitter_iter - splitter_time_vec.begin() - 1;
    split_start_time = splitter_time_vec[index_splitter];
    split_stop_time = splitter_time_vec[index_splitter + 1];
  }

  // move along the entries to find the entry inside the current splitter
  bool first_splitter_after_last_entry(false);
  if (!no_entry_in_range) {
    std::vector<DateAndTime>::iterator tsp_time_iter;
    tsp_time_iter = std::lower_bound(tsp_time_vec.begin(), tsp_time_vec.end(),
                                     split_start_time);
    if (tsp_time_iter == tsp_time_vec.end()) {
      // the first splitter's start time is LATER than the last TSP entry, then
      // there won't be any
      // TSP entry to be split into any target splitter.
      no_entry_in_range = true;
      first_splitter_after_last_entry = true;
    } else {
      // first splitter start time is between tsp_time_iter and the one before
      // it.
      // so the index for tsp_time_iter is the first TSP entry in the splitter
      index_tsp_time = tsp_time_iter - tsp_time_vec.begin();
      tsp_time = *tsp_time_iter;
    }
  } else {
    // no entry in range is true, which corresponding to the previous case
    // "already search to the last splitter which is still earlier than first
    // TSP
    // entry"
    ;
  }

  if (no_entry_in_range && first_splitter_after_last_entry) {
    // initialize all the splitters with the last value
    DateAndTime last_entry_time = this->lastTime();
    TYPE last_entry_value = this->lastValue();
    for (size_t i = 0; i < outputs.size(); ++i)
      outputs[i]->addValue(last_entry_time, last_entry_value);
  }

  // now it is the time to put TSP's entries to corresponding
  continue_search = !no_entry_in_range;
  size_t outer_while_counter = 0;
  bool partial_target_filled(false);
  while (continue_search) {
    // get next target
    int target = target_vec[index_splitter];

    // get the first entry index (overlap)
    if (index_tsp_time > 0)
      --index_tsp_time;

    // add the continous entries to same target time series property
    const size_t tspTimeVecSize = tsp_time_vec.size();
    bool continue_add = true;
    while (continue_add) {
      size_t inner_while_counter = 0;
      if (index_tsp_time == tspTimeVecSize) {
        // last entry. quit all loops
        continue_add = false;
        continue_search = false;
        partial_target_filled = true;
        break;
      }

      // add current entry
      if (outputs[target]->size() == 0 ||
          outputs[target]->lastTime() < tsp_time_vec[index_tsp_time]) {
        // avoid to add duplicate entry
        outputs[target]->addValue(m_values[index_tsp_time].time(),
                                  m_values[index_tsp_time].value());
      }

      const size_t nextTspIndex = index_tsp_time + 1;
      if (nextTspIndex < tspTimeVecSize) {
        if (tsp_time_vec[nextTspIndex] > split_stop_time) {
          // next entry is out of this splitter: add the next one and quit
          if (outputs[target]->lastTime() < m_values[nextTspIndex].time()) {
            // avoid the duplicate cases occurred in fast frequency issue
            outputs[target]->addValue(m_values[nextTspIndex].time(),
                                      m_values[nextTspIndex].value());
          }
          // FIXME - in future, need to find out WHETHER there is way to
          // skip the
          // rest without going through the whole sequence
          continue_add = false;
        }
      }

      // advance to next entry
      ++index_tsp_time;

      ++inner_while_counter;
    } // END-WHILE continue add

    // make splitters to advance to next
    ++index_splitter;
    if (index_splitter == splitter_time_vec.size() - 1) {
      // already last splitters
      continue_search = false;
    } else {
      split_start_time = split_stop_time;
      split_stop_time = splitter_time_vec[index_splitter + 1];
    }

    ++outer_while_counter;
  } // END-OF-WHILE

  // Still in 'continue search'-while-loop.  But the TSP runs over before
  // splitters.
  // Therefore, the rest of the chopper must have one more entry added!
  if (partial_target_filled) {
    // fill the target
    std::set<int> fill_target_set;
    for (size_t isplitter = index_splitter;
         isplitter < splitter_time_vec.size() - 1; ++isplitter) {
      int target_i = target_vec[isplitter];
      if (fill_target_set.find(target_i) == fill_target_set.end()) {
        if (outputs[target_i]->size() == 0 ||
            outputs[target_i]->lastTime() != m_values.back().time())
          outputs[target_i]->addValue(m_values.back().time(),
                                      m_values.back().value());
        fill_target_set.insert(target_i);
        // quit loop if it goes over all the targets
        if (fill_target_set.size() == target_set.size())
          break;
      }
    }
  }

  // Add a debugging check such that there won't be any time entry with zero log
  for (size_t i = 0; i < outputs.size(); ++i) {
    if (outputs[i]->size() == 0) {
      std::stringstream errss;
      errss << i << "-th split-out term (out of " << outputs.size()
            << " total output TSP) of '" << m_name << "'' has "
            << outputs[i]->size() << " size, whose first entry is at "
            << this->firstTime().toSimpleString();
      g_log.debug(errss.str());
    }
  }

  return;
}

// The makeFilterByValue & expandFilterToRange methods generate a bunch of
// warnings when the template type is the wider integer types
// (when it's being assigned back to a double such as in a call to minValue or
// firstValue)
// However, in reality these methods are only used for TYPE=int or double (they
// are only called from FilterByLogValue) so suppress the warnings
#ifdef _WIN32
#pragma warning(push)
#pragma warning(disable : 4244)
#pragma warning(disable : 4804) // This one comes about for TYPE=bool - again
                                // the method is never called for this type
#endif
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic ignored "-Wconversion"
#endif

/**
 * Fill a TimeSplitterType that will filter the events by matching
 * log values >= min and <= max. Creates SplittingInterval's where
 * times match the log values, and going to index==0.
 * This method is used by the FilterByLogValue algorithm.
 *
 * @param split :: Splitter that will be filled.
 * @param min :: min value
 * @param max :: max value
 * @param TimeTolerance :: offset added to times in seconds (default: 0)
 * @param centre :: Whether the log value time is considered centred or at the
 *beginning (the default).
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::makeFilterByValue(
    std::vector<SplittingInterval> &split, double min, double max,
    double TimeTolerance, bool centre) const {
  const bool emptyMin = (min == EMPTY_DBL());
  const bool emptyMax = (max == EMPTY_DBL());

  if (!emptyMin && !emptyMax && max < min) {
    std::stringstream ss;
    ss << "TimeSeriesProperty::makeFilterByValue: 'max' argument must be "
          "greater than 'min' "
       << "(got min=" << min << " max=" << max << ")";
    throw std::invalid_argument(ss.str());
  }

  // If min or max were unset ("empty") in the algorithm, set to the min or max
  // value of the log
  if (emptyMin)
    min = minValue();
  if (emptyMax)
    max = maxValue();

  // Make sure the splitter starts out empty
  split.clear();

  // Do nothing if the log is empty.
  if (m_values.empty())
    return;

  // 1. Sort
  sortIfNecessary();

  // 2. Do the rest
  bool lastGood(false);
  time_duration tol = DateAndTime::durationFromSeconds(TimeTolerance);
  int numgood = 0;
  DateAndTime t;
  DateAndTime start, stop;

  for (size_t i = 0; i < m_values.size(); ++i) {
    const DateAndTime lastTime = t;
    // The new entry
    t = m_values[i].time();
    TYPE val = m_values[i].value();

    // A good value?
    const bool isGood = ((val >= min) && (val <= max));
    if (isGood)
      numgood++;

    if (isGood != lastGood) {
      // We switched from bad to good or good to bad

      if (isGood) {
        // Start of a good section. Subtract tolerance from the time if
        // boundaries are centred.
        start = centre ? t - tol : t;
      } else {
        // End of the good section. Add tolerance to the LAST GOOD time if
        // boundaries are centred.
        // Otherwise, use the first 'bad' time.
        stop = centre ? lastTime + tol : t;
        split.emplace_back(start, stop, 0);
        // Reset the number of good ones, for next time
        numgood = 0;
      }
      lastGood = isGood;
    }
  }

  if (numgood > 0) {
    // The log ended on "good" so we need to close it using the last time we
    // found
    stop = t + tol;
    split.emplace_back(start, stop, 0);
  }
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
void TimeSeriesProperty<std::string>::makeFilterByValue(
    std::vector<SplittingInterval> & /*split*/, double /*min*/, double /*max*/,
    double /*TimeTolerance*/, bool /*centre*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::makeFilterByValue "
                                       "is not implemented for string "
                                       "properties");
}

/** If the first and/or last values in a log are between min & max, expand and
 * existing TimeSplitter
 *  (created by makeFilterByValue) if necessary to cover the full TimeInterval
 * given.
 *  This method is used by the FilterByLogValue algorithm.
 *  @param split The splitter to modify if necessary
 *  @param min   The minimum 'good' value
 *  @param max   The maximum 'good' value
 *  @param range The full time range that we want this splitter to cover
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::expandFilterToRange(
    std::vector<SplittingInterval> &split, double min, double max,
    const TimeInterval &range) const {
  const bool emptyMin = (min == EMPTY_DBL());
  const bool emptyMax = (max == EMPTY_DBL());

  if (!emptyMin && !emptyMax && max < min) {
    std::stringstream ss;
    ss << "TimeSeriesProperty::expandFilterToRange: 'max' argument must be "
          "greater than 'min' "
       << "(got min=" << min << " max=" << max << ")";
    throw std::invalid_argument(ss.str());
  }

  // If min or max were unset ("empty") in the algorithm, set to the min or max
  // value of the log
  if (emptyMin)
    min = minValue();
  if (emptyMax)
    max = maxValue();

  // Assume everything before the 1st value is constant
  double val = firstValue();
  if ((val >= min) && (val <= max)) {
    TimeSplitterType extraFilter;
    extraFilter.emplace_back(range.begin(), firstTime(), 0);
    // Include everything from the start of the run to the first time measured
    // (which may be a null time interval; this'll be ignored)
    split = split | extraFilter;
  }

  // Assume everything after the LAST value is constant
  val = lastValue();
  if ((val >= min) && (val <= max)) {
    TimeSplitterType extraFilter;
    extraFilter.emplace_back(lastTime(), range.end(), 0);
    // Include everything from the start of the run to the first time measured
    // (which may be a null time interval; this'll be ignored)
    split = split | extraFilter;
  }
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
void TimeSeriesProperty<std::string>::expandFilterToRange(
    std::vector<SplittingInterval> & /*split*/, double /*min*/, double /*max*/,
    const TimeInterval & /*range*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::makeFilterByValue "
                                       "is not implemented for string "
                                       "properties");
}

/** Calculates the time-weighted average of a property.
 *  @return The time-weighted average value of the log.
 */
template <typename TYPE>
double TimeSeriesProperty<TYPE>::timeAverageValue() const {
  double retVal = 0.0;
  try {
    const auto &filter = getSplittingIntervals();
    retVal = this->averageValueInFilter(filter);
  } catch (std::exception &) {
    // just return nan
    retVal = std::numeric_limits<double>::quiet_NaN();
  }
  return retVal;
}

/** Calculates the time-weighted average of a property in a filtered range.
 *  This is written for that case of logs whose values start at the times given.
 *  @param filter The splitter/filter restricting the range of values included
 *  @return The time-weighted average value of the log in the range within the
 * filter.
 */
template <typename TYPE>
double TimeSeriesProperty<TYPE>::averageValueInFilter(
    const std::vector<SplittingInterval> &filter) const {
  // TODO: Consider logs that aren't giving starting values.

  // First of all, if the log or the filter is empty, return NaN
  if (realSize() == 0 || filter.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  // If there's just a single value in the log, return that.
  if (realSize() == 1) {
    return static_cast<double>(m_values.front().value());
  }

  sortIfNecessary();

  double numerator(0.0), totalTime(0.0);
  // Loop through the filter ranges
  for (const auto &time : filter) {
    // Calculate the total time duration (in seconds) within by the filter
    totalTime += time.duration();

    // Get the log value and index at the start time of the filter
    int index;
    double value = getSingleValue(time.start(), index);
    DateAndTime startTime = time.start();

    while (index < realSize() - 1 && m_values[index + 1].time() < time.stop()) {
      ++index;
      numerator +=
          DateAndTime::secondsFromDuration(m_values[index].time() - startTime) *
          value;
      startTime = m_values[index].time();
      value = static_cast<double>(m_values[index].value());
    }

    // Now close off with the end of the current filter range
    numerator +=
        DateAndTime::secondsFromDuration(time.stop() - startTime) * value;
  }

  // 'Normalise' by the total time
  return numerator / totalTime;
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
double TimeSeriesProperty<std::string>::averageValueInFilter(
    const TimeSplitterType & /*filter*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::"
                                       "averageValueInFilter is not "
                                       "implemented for string properties");
}

template <typename TYPE>
std::pair<double, double>
TimeSeriesProperty<TYPE>::timeAverageValueAndStdDev() const {
  std::pair<double, double> retVal{0., 0.}; // mean and stddev
  try {
    const auto &filter = getSplittingIntervals();
    retVal = this->averageAndStdDevInFilter(filter);
  } catch (std::exception &) {
    retVal.first = std::numeric_limits<double>::quiet_NaN();
    retVal.second = std::numeric_limits<double>::quiet_NaN();
  }
  return retVal;
}

template <typename TYPE>
std::pair<double, double> TimeSeriesProperty<TYPE>::averageAndStdDevInFilter(
    const std::vector<SplittingInterval> &filter) const {
  // the mean to calculate the standard deviation about
  // this will sort the log as necessary as well
  const double mean = this->averageValueInFilter(filter);

  // First of all, if the log or the filter is empty or is a single value,
  // return NaN for the uncertainty
  if (realSize() <= 1 || filter.empty()) {
    return std::pair<double, double>{mean,
                                     std::numeric_limits<double>::quiet_NaN()};
  }

  double numerator(0.0), totalTime(0.0);
  // Loop through the filter ranges
  for (const auto &time : filter) {
    // Calculate the total time duration (in seconds) within by the filter
    totalTime += time.duration();

    // Get the log value and index at the start time of the filter
    int index;
    double value = getSingleValue(time.start(), index);
    double valuestddev = (value - mean) * (value - mean);
    DateAndTime startTime = time.start();

    while (index < realSize() - 1 && m_values[index + 1].time() < time.stop()) {
      ++index;

      numerator +=
          DateAndTime::secondsFromDuration(m_values[index].time() - startTime) *
          valuestddev;
      startTime = m_values[index].time();
      value = static_cast<double>(m_values[index].value());
      valuestddev = (value - mean) * (value - mean);
    }

    // Now close off with the end of the current filter range
    numerator +=
        DateAndTime::secondsFromDuration(time.stop() - startTime) * valuestddev;
  }

  // Normalise by the total time
  return std::pair<double, double>{mean, std::sqrt(numerator / totalTime)};
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
std::pair<double, double>
TimeSeriesProperty<std::string>::averageAndStdDevInFilter(
    const TimeSplitterType & /*filter*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::"
                                       "averageAndStdDevInFilter is not "
                                       "implemented for string properties");
}

// Re-enable the warnings disabled before makeFilterByValue
#ifdef _WIN32
#pragma warning(pop)
#endif
#if defined(__GNUC__) && !(defined(__INTEL_COMPILER))
#pragma GCC diagnostic warning "-Wconversion"
#endif

/**
 *  Return the time series as a correct C++ map<DateAndTime, TYPE>. All values
 * are included.
 *
 * @return time series property values as map
 */
template <typename TYPE>
std::map<DateAndTime, TYPE>
TimeSeriesProperty<TYPE>::valueAsCorrectMap() const {
  // 1. Sort if necessary
  sortIfNecessary();

  // 2. Data Strcture
  std::map<DateAndTime, TYPE> asMap;

  if (!m_values.empty()) {
    for (size_t i = 0; i < m_values.size(); i++)
      asMap[m_values[i].time()] = m_values[i].value();
  }

  return asMap;
}

/**
 *  Return the time series's values as a vector<TYPE>
 *  @return the time series's values as a vector<TYPE>
 */
template <typename TYPE>
std::vector<TYPE> TimeSeriesProperty<TYPE>::valuesAsVector() const {
  sortIfNecessary();

  std::vector<TYPE> out;
  out.reserve(m_values.size());

  for (size_t i = 0; i < m_values.size(); i++)
    out.push_back(m_values[i].value());

  return out;
}

/**
 * Return the time series as a C++ multimap<DateAndTime, TYPE>. All values.
 * This method is used in parsing the ISIS ICPevent log file: different
 * commands
 * can be recorded against the same time stamp but all must be present.
 */
template <typename TYPE>
std::multimap<DateAndTime, TYPE>
TimeSeriesProperty<TYPE>::valueAsMultiMap() const {
  std::multimap<DateAndTime, TYPE> asMultiMap;

  if (!m_values.empty()) {
    for (size_t i = 0; i < m_values.size(); i++)
      asMultiMap.insert(
          std::make_pair(m_values[i].time(), m_values[i].value()));
  }

  return asMultiMap;
}

/**
 * Return the time series's times as a vector<DateAndTime>
 * @return A vector of DateAndTime objects
 */
template <typename TYPE>
std::vector<DateAndTime> TimeSeriesProperty<TYPE>::timesAsVector() const {
  sortIfNecessary();

  std::vector<DateAndTime> out;
  out.reserve(m_values.size());

  for (size_t i = 0; i < m_values.size(); i++) {
    out.push_back(m_values[i].time());
  }

  return out;
}

/**
 * @return Return the series as list of times, where the time is the number of
 * seconds since the start.
 */
template <typename TYPE>
std::vector<double> TimeSeriesProperty<TYPE>::timesAsVectorSeconds() const {
  // 1. Sort if necessary
  sortIfNecessary();

  // 2. Output data structure
  std::vector<double> out;
  out.reserve(m_values.size());

  Types::Core::DateAndTime start = m_values[0].time();
  for (size_t i = 0; i < m_values.size(); i++) {
    out.push_back(DateAndTime::secondsFromDuration(m_values[i].time() - start));
  }

  return out;
}

/** Add a value to the series.
 *  Added values need not be sequential in time.
 *  @param time   The time
 *  @param value  The associated value
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::addValue(const Types::Core::DateAndTime &time,
                                        const TYPE value) {
  TimeValueUnit<TYPE> newvalue(time, value);
  // Add the value to the back of the vector
  m_values.push_back(newvalue);
  // Increment the separate record of the property's size
  m_size++;

  // Toggle the sorted flag if necessary
  // (i.e. if the flag says we're sorted and the added time is before the prior
  // last time)
  if (m_size == 1) {
    // First item, must be sorted.
    m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
  } else if (m_propSortedFlag == TimeSeriesSortStatus::TSUNKNOWN &&
             m_values.back() < *(m_values.rbegin() + 1)) {
    // Previously unknown and still unknown
    m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
  } else if (m_propSortedFlag == TimeSeriesSortStatus::TSSORTED &&
             m_values.back() < *(m_values.rbegin() + 1)) {
    // Previously sorted but last added is not in order
    m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
  }

  m_filterApplied = false;
}

/** Add a value to the map
 *  @param time :: The time as a string in the format: (ISO 8601)
 * yyyy-mm-ddThh:mm:ss
 *  @param value :: The associated value
 *  @return True if insertion successful (i.e. identical time not already in map
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::addValue(const std::string &time,
                                        const TYPE value) {
  return addValue(Types::Core::DateAndTime(time), value);
}

/**
 * Add a value to the map using a time_t
 *  @param time :: The time as a time_t value
 *  @param value :: The associated value
 *  @return True if insertion successful (i.e. identical time not already in map
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::addValue(const std::time_t &time,
                                        const TYPE value) {
  Types::Core::DateAndTime dt;
  dt.set_from_time_t(time);
  return addValue(dt, value);
}

/** Adds vectors of values to the map. Should be much faster than repeated calls
 * to addValue.
 *  @param times :: The time as a boost::posix_time::ptime value
 *  @param values :: The associated value
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::addValues(
    const std::vector<Types::Core::DateAndTime> &times,
    const std::vector<TYPE> &values) {
  size_t length = std::min(times.size(), values.size());
  m_size += static_cast<int>(length);
  for (size_t i = 0; i < length; ++i) {
    m_values.emplace_back(times[i], values[i]);
  }

  if (!values.empty())
    m_propSortedFlag = TimeSeriesSortStatus::TSUNKNOWN;
}

/** replace vectors of values to the map. First we clear the vectors
 * and then we run addValues
 *  @param times :: The time as a boost::posix_time::ptime value
 *  @param values :: The associated value
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::replaceValues(
    const std::vector<Types::Core::DateAndTime> &times,
    const std::vector<TYPE> &values) {
  clear();
  addValues(times, values);
}

/**
 * Returns the last time
 * @return Value
 */
template <typename TYPE>
DateAndTime TimeSeriesProperty<TYPE>::lastTime() const {
  if (m_values.empty()) {
    const std::string error("lastTime(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  sortIfNecessary();

  return m_values.rbegin()->time();
}

/** Returns the first value regardless of filter
 *  @return Value
 */
template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::firstValue() const {
  if (m_values.empty()) {
    const std::string error("firstValue(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  sortIfNecessary();

  return m_values[0].value();
}

/** Returns the first time regardless of filter
 *  @return Value
 */
template <typename TYPE>
DateAndTime TimeSeriesProperty<TYPE>::firstTime() const {
  if (m_values.empty()) {
    const std::string error("firstTime(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  sortIfNecessary();

  return m_values[0].time();
}

/**
 * Returns the last value
 *  @return Value
 */
template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::lastValue() const {
  if (m_values.empty()) {
    const std::string error("lastValue(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  sortIfNecessary();

  return m_values.rbegin()->value();
}

template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::minValue() const {
  return std::min_element(m_values.begin(), m_values.end(),
                          TimeValueUnit<TYPE>::valueCmp)
      ->value();
}

template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::maxValue() const {
  return std::max_element(m_values.begin(), m_values.end(),
                          TimeValueUnit<TYPE>::valueCmp)
      ->value();
}

/// Returns the number of values at UNIQUE time intervals in the time series
/// @returns The number of unique time interfaces
template <typename TYPE> int TimeSeriesProperty<TYPE>::size() const {
  return m_size;
}

/**
 * Returns the real size of the time series property map:
 * the number of entries, including repeated ones.
 */
template <typename TYPE> int TimeSeriesProperty<TYPE>::realSize() const {
  return static_cast<int>(m_values.size());
}

/*
 * Get the time series property as a string of 'time  value'
 * @return time series property as a string
 */
template <typename TYPE> std::string TimeSeriesProperty<TYPE>::value() const {
  sortIfNecessary();

  std::stringstream ins;
  for (size_t i = 0; i < m_values.size(); i++) {
    try {
      ins << m_values[i].time().toSimpleString();
      ins << "  " << m_values[i].value() << "\n";
    } catch (...) {
      // Some kind of error; for example, invalid year, can occur when
      // converting boost time.
      ins << "Error Error"
          << "\n";
    }
  }

  return ins.str();
}

/**  New method to return time series value pairs as std::vector<std::string>
 *
 * @return time series property values as a string vector "<time_t> value"
 */
template <typename TYPE>
std::vector<std::string> TimeSeriesProperty<TYPE>::time_tValue() const {
  sortIfNecessary();

  std::vector<std::string> values;
  values.reserve(m_values.size());

  for (size_t i = 0; i < m_values.size(); i++) {
    std::stringstream line;
    line << m_values[i].time().toSimpleString() << " " << m_values[i].value();
    values.push_back(line.str());
  }

  return values;
}

/**
 * Return the time series as a C++ map<DateAndTime, TYPE>
 *
 * WARNING: THIS ONLY RETURNS UNIQUE VALUES, AND SKIPS ANY REPEATED VALUES!
 *   USE AT YOUR OWN RISK! Try valueAsCorrectMap() instead.
 * @return time series property values as map
 */
template <typename TYPE>
std::map<DateAndTime, TYPE> TimeSeriesProperty<TYPE>::valueAsMap() const {
  // 1. Sort if necessary
  sortIfNecessary();

  // 2. Build map

  std::map<DateAndTime, TYPE> asMap;
  if (m_values.empty())
    return asMap;

  TYPE d = m_values[0].value();
  asMap[m_values[0].time()] = d;

  for (size_t i = 1; i < m_values.size(); i++) {
    if (m_values[i].value() != d) {
      // Only put entry with different value from last entry to map
      asMap[m_values[i].time()] = m_values[i].value();
      d = m_values[i].value();
    }
  }
  return asMap;
}

/**
 * Set the property from a string value. Throws a NotImplementedError
 *  @throw Exception::NotImplementedError Not yet implemented
 * @return Nothing in this case
 */
template <typename TYPE>
std::string TimeSeriesProperty<TYPE>::setValue(const std::string & /*unused*/) {
  throw Exception::NotImplementedError("TimeSeriesProperty<TYPE>::setValue - "
                                       "Cannot extract TimeSeries from a "
                                       "std::string");
}

/**
 * Set the property from a Json value. Throws a NotImplementedError
 *  @throw Exception::NotImplementedError Not yet implemented
 * @return Nothing in this case
 */
template <typename TYPE>
std::string
TimeSeriesProperty<TYPE>::setValueFromJson(const Json::Value & /*unused*/) {
  throw Exception::NotImplementedError("TimeSeriesProperty<TYPE>::setValue - "
                                       "Cannot extract TimeSeries from a "
                                       "Json::Value");
}

/**
 * @throw Exception::NotImplementedError Not yet implemented
 * @return Nothing in this case
 */
template <typename TYPE>
std::string TimeSeriesProperty<TYPE>::setDataItem(
    const boost::shared_ptr<DataItem> /*unused*/) {
  throw Exception::NotImplementedError("TimeSeriesProperty<TYPE>::setValue - "
                                       "Cannot extract TimeSeries from "
                                       "DataItem");
}

/** Clears out the values in the property
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::clear() {
  m_size = 0;
  m_values.clear();

  m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
  m_filterApplied = false;
}

/** Clears out all but the last value in the property.
 *  The last value is the last entry in the m_values vector - no sorting is
 *  done or checked for to ensure that the last value is the most recent in
 * time.
 *  It is up to the client to call sortIfNecessary() first if this is a
 * requirement.
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::clearOutdated() {
  if (realSize() > 1) {
    auto lastValue = m_values.back();
    clear();
    m_values.push_back(lastValue);
    m_size = 1;
  }
}

//--------------------------------------------------------------------------------------------
/**
 * Clears and creates a TimeSeriesProperty from these parameters:
 *  @param start_time :: The reference time as a boost::posix_time::ptime value
 *  @param time_sec :: A vector of time offset (from start_time) in seconds.
 *  @param new_values :: A vector of values, each corresponding to the time
 * offset in time_sec.
 *    Vector sizes must match.
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::create(
    const Types::Core::DateAndTime &start_time,
    const std::vector<double> &time_sec, const std::vector<TYPE> &new_values) {
  if (time_sec.size() != new_values.size())
    throw std::invalid_argument("TimeSeriesProperty::create: mismatched size "
                                "for the time and values vectors.");

  // Make the times(as seconds) into a vector of DateAndTime in one go.
  std::vector<DateAndTime> times;
  DateAndTime::createVector(start_time, time_sec, times);

  this->create(times, new_values);
}

//--------------------------------------------------------------------------------------------
/** Clears and creates a TimeSeriesProperty from these parameters:
 *
 * @param new_times :: A vector of DateAndTime.
 * @param new_values :: A vector of values, each corresponding to the time off
 *set in new_time.
 *                      Vector sizes must match.
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::create(
    const std::vector<Types::Core::DateAndTime> &new_times,
    const std::vector<TYPE> &new_values) {
  if (new_times.size() != new_values.size())
    throw std::invalid_argument("TimeSeriesProperty::create: mismatched size "
                                "for the time and values vectors.");

  clear();
  m_values.reserve(new_times.size());

  std::size_t num = new_values.size();

  m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
  for (std::size_t i = 0; i < num; i++) {
    TimeValueUnit<TYPE> newentry(new_times[i], new_values[i]);
    m_values.push_back(newentry);
    if (m_propSortedFlag == TimeSeriesSortStatus::TSSORTED && i > 0 &&
        new_times[i - 1] > new_times[i]) {
      // Status gets to unsorted
      m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
    }
  }

  // reset the size
  m_size = static_cast<int>(m_values.size());
}

/** Returns the value at a particular time
 *  @param t :: time
 *  @return Value at time \a t
 */
template <typename TYPE>
TYPE TimeSeriesProperty<TYPE>::getSingleValue(
    const Types::Core::DateAndTime &t) const {
  if (m_values.empty()) {
    const std::string error("getSingleValue(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 1. Get sorted
  sortIfNecessary();

  // 2.
  TYPE value;
  if (t < m_values[0].time()) {
    // 1. Out side of lower bound
    value = m_values[0].value();
  } else if (t >= m_values.back().time()) {
    // 2. Out side of upper bound
    value = m_values.back().value();
  } else {
    // 3. Within boundary
    int index = this->findIndex(t);

    if (index < 0) {
      // If query time "t" is earlier than the begin time of the series
      index = 0;
    } else if (index == int(m_values.size())) {
      // If query time "t" is later than the end time of the  series
      index = static_cast<int>(m_values.size()) - 1;
    } else if (index > int(m_values.size())) {
      std::stringstream errss;
      errss << "TimeSeriesProperty.findIndex() returns index (" << index
            << " ) > maximum defined value " << m_values.size();
      throw std::logic_error(errss.str());
    }

    value = m_values[static_cast<size_t>(index)].value();
  }

  return value;
} // END-DEF getSinglevalue()

/** Returns the value at a particular time
 *  @param t :: time
 *  @param index :: index of time
 *  @return Value at time \a t
 */
template <typename TYPE>
TYPE TimeSeriesProperty<TYPE>::getSingleValue(const Types::Core::DateAndTime &t,
                                              int &index) const {
  if (m_values.empty()) {
    const std::string error("getSingleValue(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 1. Get sorted
  sortIfNecessary();

  // 2.
  TYPE value;
  if (t < m_values[0].time()) {
    // 1. Out side of lower bound
    value = m_values[0].value();
    index = 0;
  } else if (t >= m_values.back().time()) {
    // 2. Out side of upper bound
    value = m_values.back().value();
    index = int(m_values.size()) - 1;
  } else {
    // 3. Within boundary
    index = this->findIndex(t);

    if (index < 0) {
      // If query time "t" is earlier than the begin time of the series
      index = 0;
    } else if (index == int(m_values.size())) {
      // If query time "t" is later than the end time of the  series
      index = static_cast<int>(m_values.size()) - 1;
    } else if (index > int(m_values.size())) {
      std::stringstream errss;
      errss << "TimeSeriesProperty.findIndex() returns index (" << index
            << " ) > maximum defined value " << m_values.size();
      throw std::logic_error(errss.str());
    }

    value = m_values[static_cast<size_t>(index)].value();
  }

  return value;
} // END-DEF getSinglevalue()

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
template <typename TYPE>
TimeInterval TimeSeriesProperty<TYPE>::nthInterval(int n) const {
  // 0. Throw exception
  if (m_values.empty()) {
    const std::string error("nthInterval(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 1. Sort
  sortIfNecessary();

  // 2. Calculate time interval

  Kernel::TimeInterval deltaT;

  if (m_filter.empty()) {
    // I. No filter
    if (n >= static_cast<int>(m_values.size()) ||
        (n == static_cast<int>(m_values.size()) - 1 && m_values.size() == 1)) {
      // 1. Out of bound
      ;
    } else if (n == static_cast<int>(m_values.size()) - 1) {
      // 2. Last one by making up an end time.
      time_duration d =
          m_values.rbegin()->time() - (m_values.rbegin() + 1)->time();
      DateAndTime endTime = m_values.rbegin()->time() + d;
      Kernel::TimeInterval dt(m_values.rbegin()->time(), endTime);
      deltaT = dt;
    } else {
      // 3. Regular
      DateAndTime startT = m_values[static_cast<std::size_t>(n)].time();
      DateAndTime endT = m_values[static_cast<std::size_t>(n) + 1].time();
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
      long ind_t1 = static_cast<long>(m_filterQuickRef.back().first);
      long ind_t2 = ind_t1 - 1;
      Types::Core::DateAndTime t1 = (m_values.begin() + ind_t1)->time();
      Types::Core::DateAndTime t2 = (m_values.begin() + ind_t2)->time();
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
      Types::Core::DateAndTime ftime0 =
          m_filter[m_filterQuickRef[refindex].first].first;
      size_t iStartIndex =
          m_filterQuickRef[refindex + 1].first + static_cast<size_t>(diff);
      Types::Core::DateAndTime ltime0 = m_values[iStartIndex].time();
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
      if (iStopIndex >= m_values.size()) {
        // a) Last log entry is for the start
        Types::Core::DateAndTime ftimef =
            m_filter[m_filterQuickRef[refindex + 3].first].first;
        tf = ftimef;
      } else {
        // b) Using the earlier value of next log entry and next filter entry
        Types::Core::DateAndTime ltimef = m_values[iStopIndex].time();
        Types::Core::DateAndTime ftimef =
            m_filter[m_filterQuickRef[refindex + 3].first].first;
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
template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::nthValue(int n) const {
  TYPE value;

  // 1. Throw error if property is empty
  if (m_values.empty()) {
    const std::string error("nthValue(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 2. Sort and apply filter
  sortIfNecessary();

  if (m_filter.empty()) {
    // 3. Situation 1:  No filter
    if (static_cast<size_t>(n) < m_values.size()) {
      TimeValueUnit<TYPE> entry = m_values[static_cast<std::size_t>(n)];
      value = entry.value();
    } else {
      TimeValueUnit<TYPE> entry =
          m_values[static_cast<std::size_t>(m_size) - 1];
      value = entry.value();
    }
  } else {
    // 4. Situation 2: There is filter
    this->applyFilter();

    if (static_cast<size_t>(n) > m_filterQuickRef.back().second + 1) {
      // 1. n >= size of the allowed region
      size_t ilog = (m_filterQuickRef.rbegin() + 1)->first;
      value = m_values[ilog].value();
    } else {
      // 2. n < size
      Types::Core::DateAndTime t0;
      Types::Core::DateAndTime tf;

      size_t refindex = findNthIndexFromQuickRef(n);
      if (refindex + 3 >= m_filterQuickRef.size()) {
        throw std::logic_error("Not consider out of boundary case here. ");
      }
      size_t ilog =
          m_filterQuickRef[refindex + 1].first +
          (static_cast<std::size_t>(n) - m_filterQuickRef[refindex].second);
      value = m_values[ilog].value();
    } // END-IF-ELSE Cases
  }

  return value;
}

/** Returns n-th time, or the last time if fewer than n entries.
 *  Special cases: There is no special cases
 *  @param n :: index
 *  @return DateAndTime
 */
template <typename TYPE>
Types::Core::DateAndTime TimeSeriesProperty<TYPE>::nthTime(int n) const {
  sortIfNecessary();

  if (m_values.empty()) {
    const std::string error("nthTime(): TimeSeriesProperty '" + name() +
                            "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  if (n < 0 || n >= static_cast<int>(m_values.size()))
    n = static_cast<int>(m_values.size()) - 1;

  return m_values[static_cast<size_t>(n)].time();
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
template <typename TYPE>
void TimeSeriesProperty<TYPE>::filterWith(
    const TimeSeriesProperty<bool> *filter) {
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
    if (m_values.back().time() > filtertimes.back()) {
      const size_t nvalues(m_values.size());
      // Last log time is later than last filter time
      lastTime = m_values.back().time();
      if (nvalues > 1 && m_values[nvalues - 2].time() > filtertimes.back())
        nextLastT = m_values[nvalues - 2].time();
      else
        nextLastT = filtertimes.back();
    } else {
      // Last log time is no later than last filter time
      lastTime = filtertimes.back();
      const size_t nfilterValues(filtervalues.size());
      // If last-but-one filter time is still later than value then previous is
      // this
      // else it is the last value time
      if (nfilterValues > 1 &&
          m_values.back().time() > filtertimes[nfilterValues - 2])
        nextLastT = filtertimes[nfilterValues - 2];
      else
        nextLastT = m_values.back().time();
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
template <typename TYPE> void TimeSeriesProperty<TYPE>::clearFilter() {
  m_filter.clear();
  m_filterQuickRef.clear();
}

/**
 * Updates size()
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::countSize() const {
  if (m_filter.empty()) {
    // 1. Not filter
    m_size = int(m_values.size());
  } else {
    // 2. With Filter
    if (!m_filterApplied) {
      this->applyFilter();
    }
    size_t nvalues = m_filterQuickRef.empty() ? m_values.size()
                                              : m_filterQuickRef.back().second;
    m_size = static_cast<int>(nvalues);
  }
}

/**  Check if str has the right time format
 *   @param str :: The string to check
 *   @return True if the format is correct, false otherwise.
 */
template <typename TYPE>
bool TimeSeriesProperty<TYPE>::isTimeString(const std::string &str) {
  static const boost::regex re(
      "^[0-9]{4}.[0-9]{2}.[0-9]{2}.[0-9]{2}.[0-9]{2}.[0-9]{2}");
  return boost::regex_search(str.begin(), str.end(), re);
}

/**
 * This doesn't check anything -we assume these are always valid
 *  @returns an empty string ""
 */
template <typename TYPE> std::string TimeSeriesProperty<TYPE>::isValid() const {
  return "";
}

/*
 * A TimeSeriesProperty never has a default, so return empty string
 * @returns Empty string as no defaults can be provided
 */
template <typename TYPE>
std::string TimeSeriesProperty<TYPE>::getDefault() const {
  return ""; // No defaults can be provided=empty string
}

/**
 * A TimeSeriesProperty never has a default
 */
template <typename TYPE> bool TimeSeriesProperty<TYPE>::isDefault() const {
  return false;
}

/**
 * Return a TimeSeriesPropertyStatistics struct containing the
 * statistics of this TimeSeriesProperty object.
 *
 * N.B. This method DOES take filtering into account
 */
template <typename TYPE>
TimeSeriesPropertyStatistics TimeSeriesProperty<TYPE>::getStatistics() const {
  TimeSeriesPropertyStatistics out;
  Mantid::Kernel::Statistics raw_stats =
      Mantid::Kernel::getStatistics(this->filteredValuesAsVector());
  out.mean = raw_stats.mean;
  out.standard_deviation = raw_stats.standard_deviation;
  out.median = raw_stats.median;
  out.minimum = raw_stats.minimum;
  out.maximum = raw_stats.maximum;
  if (this->size() > 0) {
    const auto &intervals = this->getSplittingIntervals();
    const double duration_sec =
        std::accumulate(intervals.cbegin(), intervals.cend(), 0.,
                        [](double sum, const auto &interval) {
                          return sum + interval.duration();
                        });
    out.duration = duration_sec;
    const auto time_weighted = this->timeAverageValueAndStdDev();
    out.time_mean = time_weighted.first;
    out.time_standard_deviation = time_weighted.second;
  } else {
    out.time_mean = std::numeric_limits<double>::quiet_NaN();
    out.time_standard_deviation = std::numeric_limits<double>::quiet_NaN();
    out.duration = std::numeric_limits<double>::quiet_NaN();
  }

  return out;
}

/*
 * Detects whether there are duplicated entries (of time) in property
 * If there is any, keep one of them
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::eliminateDuplicates() {
  // 1. Sort if necessary
  sortIfNecessary();

  // 2. Detect and Remove Duplicated
  size_t numremoved = 0;

  typename std::vector<TimeValueUnit<TYPE>>::iterator vit;
  vit = m_values.begin() + 1;
  Types::Core::DateAndTime prevtime = m_values.begin()->time();
  while (vit != m_values.end()) {
    Types::Core::DateAndTime currtime = vit->time();
    if (prevtime == currtime) {
      // Print out warning
      g_log.debug() << "Entry @ Time = " << prevtime
                    << "has duplicate time stamp.  Remove entry with Value = "
                    << (vit - 1)->value() << "\n";

      // A duplicated entry!
      vit = m_values.erase(vit - 1);

      numremoved++;
    }

    // b) progress
    prevtime = currtime;
    ++vit;
  }

  // update m_size
  countSize();

  // 3. Finish
  g_log.warning() << "Log " << this->name() << " has " << numremoved
                  << " entries removed due to duplicated time. "
                  << "\n";
}

/*
 * Print the content to string
 */
template <typename TYPE>
std::string TimeSeriesProperty<TYPE>::toString() const {
  std::stringstream ss;
  for (size_t i = 0; i < m_values.size(); ++i)
    ss << m_values[i].time() << "\t\t" << m_values[i].value() << "\n";

  return ss.str();
}

//-------------------------------------------------------------------------
// Private methods
//-------------------------------------------------------------------------

//----------------------------------------------------------------------------------
/*
 * Sort vector mP and set the flag. Only sorts if the values are not already
 * sorted.
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::sortIfNecessary() const {
  if (m_propSortedFlag == TimeSeriesSortStatus::TSUNKNOWN) {
    bool sorted = is_sorted(m_values.begin(), m_values.end());
    if (sorted)
      m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
    else
      m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
  }

  if (m_propSortedFlag == TimeSeriesSortStatus::TSUNSORTED) {
    g_log.information(
        "TimeSeriesProperty is not sorted.  Sorting is operated on it. ");
    std::stable_sort(m_values.begin(), m_values.end());
    m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
  }
}

/** Find the index of the entry of time t in the mP vector (sorted)
 *  Return @ if t is within log.begin and log.end, then the index of the log
 * equal or just smaller than t
 *           if t is earlier (less) than the starting time, return -1
 *           if t is later (larger) than the ending time, return m_value.size
 */
template <typename TYPE>
int TimeSeriesProperty<TYPE>::findIndex(Types::Core::DateAndTime t) const {
  // 0. Return with an empty container
  if (m_values.empty())
    return 0;

  // 1. Sort
  sortIfNecessary();

  // 2. Extreme value
  if (t <= m_values[0].time()) {
    return -1;
  } else if (t >= m_values.back().time()) {
    return (int(m_values.size()));
  }

  // 3. Find by lower_bound()
  typename std::vector<TimeValueUnit<TYPE>>::const_iterator fid;
  TimeValueUnit<TYPE> temp(t, m_values[0].value());
  fid = std::lower_bound(m_values.begin(), m_values.end(), temp);

  int newindex = int(fid - m_values.begin());
  if (fid->time() > t)
    newindex--;

  return newindex;
}

/** Find the upper_bound of time t in container.
 * Search range:  begin+istart to begin+iend
 * Return C[ir] == t or C[ir] > t and C[ir-1] < t
 *        -1:          exceeding lower bound
 *        mP.size():   exceeding upper bound
 */
template <typename TYPE>
int TimeSeriesProperty<TYPE>::upperBound(Types::Core::DateAndTime t, int istart,
                                         int iend) const {
  // 0. Check validity
  if (istart < 0) {
    throw std::invalid_argument("Start Index cannot be less than 0");
  }
  if (iend >= static_cast<int>(m_values.size())) {
    throw std::invalid_argument("End Index cannot exceed the boundary");
  }
  if (istart > iend) {
    throw std::invalid_argument("Start index cannot be greater than end index");
  }

  // 1. Return instantly if it is out of boundary
  if (t < (m_values.begin() + istart)->time()) {
    return -1;
  }
  if (t > (m_values.begin() + iend)->time()) {
    return static_cast<int>(m_values.size());
  }

  // 2. Sort
  sortIfNecessary();

  // 3. Construct the pair for comparison and do lower_bound()
  TimeValueUnit<TYPE> temppair(t, m_values[0].value());
  typename std::vector<TimeValueUnit<TYPE>>::iterator fid;
  fid = std::lower_bound((m_values.begin() + istart),
                         (m_values.begin() + iend + 1), temppair);
  if (fid == m_values.end())
    throw std::runtime_error("Cannot find data");

  // 4. Calculate return value
  size_t index = size_t(fid - m_values.begin());

  return int(index);
}

/*
 * Apply filter
 * Requirement: There is no 2 consecutive 'second' values that are same in
 *mFilter
 *
 * It only works with filter starting from TRUE AND having TRUE and FALSE
 *altered
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::applyFilter() const {
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

      if (icurlog < static_cast<int>(m_values.size()))
        icurlog = this->upperBound(m_filter[ift].first, istart,
                                   static_cast<int>(m_values.size()) - 1);

      if (icurlog < 0) {
        // i. If it is out of lower boundary, add filter time, add 0 time
        if (!m_filterQuickRef.empty())
          throw std::logic_error(
              "return log index < 0 only occurs with the first log entry");

        m_filterQuickRef.emplace_back(ift, 0);
        m_filterQuickRef.emplace_back(0, 0);

        icurlog = 0;
      } else if (icurlog >= static_cast<int>(m_values.size())) {
        // ii.  If it is out of upper boundary, still record it.  but make the
        // log entry to mP.size()+1
        size_t ip = 0;
        if (m_filterQuickRef.size() >= 4)
          ip = m_filterQuickRef.back().second;
        m_filterQuickRef.emplace_back(ift, ip);
        m_filterQuickRef.emplace_back(m_values.size() + 1, ip);
      } else {
        // iii. The returned value is in the boundary.
        size_t numintervals = 0;
        if (!m_filterQuickRef.empty()) {
          numintervals = m_filterQuickRef.back().second;
        }
        if (m_filter[ift].first <
            m_values[static_cast<std::size_t>(icurlog)].time()) {
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

      if (ilastlog < static_cast<int>(m_values.size())) {
        // B1: Last TRUE entry is still within log
        icurlog = this->upperBound(m_filter[ift].first, icurlog,
                                   static_cast<int>(m_values.size()) - 1);

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
            throw std::logic_error("LastLog is not expected to be less than 0");

          int delta_numintervals = icurlog - ilastlog;
          if (delta_numintervals < 0)
            throw std::logic_error(
                "Havn't considered delta numinterval can be less than 0.");

          size_t new_numintervals = m_filterQuickRef.back().second +
                                    static_cast<size_t>(delta_numintervals);

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
template <typename TYPE>
size_t TimeSeriesProperty<TYPE>::findNthIndexFromQuickRef(int n) const {
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
template <typename TYPE>
std::string
TimeSeriesProperty<TYPE>::setValueFromProperty(const Property &right) {
  auto prop = dynamic_cast<const TimeSeriesProperty<TYPE> *>(&right);
  if (!prop) {
    return "Could not set value: properties have different type.";
  }
  m_values = prop->m_values;
  m_size = prop->m_size;
  m_propSortedFlag = prop->m_propSortedFlag;
  m_filter = prop->m_filter;
  m_filterQuickRef = prop->m_filterQuickRef;
  m_filterApplied = prop->m_filterApplied;
  return "";
}

//----------------------------------------------------------------------------------------------
/** Saves the time vector has time + start attribute */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::saveTimeVector(::NeXus::File *file) {
  std::vector<DateAndTime> times = this->timesAsVector();
  const DateAndTime &start = times.front();
  std::vector<double> timeSec(times.size());
  for (size_t i = 0; i < times.size(); i++)
    timeSec[i] = static_cast<double>(times[i].totalNanoseconds() -
                                     start.totalNanoseconds()) *
                 1e-9;
  file->writeData("time", timeSec);
  file->openData("time");
  file->putAttr("start", start.toISO8601String());
  file->closeData();
}

//----------------------------------------------------------------------------------------------
/** Helper function to save a TimeSeriesProperty<> */
template <>
void TimeSeriesProperty<std::string>::saveProperty(::NeXus::File *file) {
  std::vector<std::string> values = this->valuesAsVector();
  if (values.empty())
    return;
  file->makeGroup(this->name(), "NXlog", true);

  // Find the max length of any string
  auto max_it =
      std::max_element(values.begin(), values.end(),
                       [](const std::string &a, const std::string &b) {
                         return a.size() < b.size();
                       });
  // Increment by 1 to have the 0 terminator
  size_t maxlen = max_it->size() + 1;
  // Copy into one array
  std::vector<char> strs(values.size() * maxlen);
  size_t index = 0;
  for (const auto &prop : values) {
    std::copy(prop.begin(), prop.end(), &strs[index]);
    index += maxlen;
  }

  std::vector<int> dims{static_cast<int>(values.size()),
                        static_cast<int>(maxlen)};
  file->makeData("value", ::NeXus::CHAR, dims, true);
  file->putData(strs.data());
  file->closeData();
  saveTimeVector(file);
  file->closeGroup();
}

/**
 * Helper function to save a TimeSeriesProperty<bool>
 * At the time of writing NeXus does not support boolean directly. We will use a
 * UINT8
 * for the value and add an attribute boolean to inidcate it is actually a bool
 */
template <> void TimeSeriesProperty<bool>::saveProperty(::NeXus::File *file) {
  std::vector<bool> value = this->valuesAsVector();
  if (value.empty())
    return;
  std::vector<uint8_t> asUint(value.begin(), value.end());
  file->makeGroup(this->name(), "NXlog", true);
  file->writeData("value", asUint);
  file->putAttr("boolean", "1");
  saveTimeVector(file);
  file->closeGroup();
}

template <typename TYPE>
void TimeSeriesProperty<TYPE>::saveProperty(::NeXus::File *file) {
  auto value = this->valuesAsVector();
  if (value.empty())
    return;
  file->makeGroup(this->name(), "NXlog", 1);
  file->writeData("value", value);
  file->openData("value");
  file->putAttr("units", this->units());
  file->closeData();
  saveTimeVector(file);
  file->closeGroup();
}

/**
 * @returns the value as a Json object. The string representation is
 * used as the underlying type
 */
template <typename TYPE>
Json::Value TimeSeriesProperty<TYPE>::valueAsJson() const {
  return Json::Value(value());
}

/** Calculate constant step histogram of the time series data.
 * @param tMin    -- minimal time to include in histogram
 * @param tMax    -- maximal time to constrain the histogram data
 * @param counts  -- vector of output histogrammed data.
 *   On input, the size of the vector defines the number of points in the
 *   histogram.
 *   On output, adds all property elements belonging to the time interval
 *  [tMin+n*dT;tMin+(n+1)*dT]
 *  to the initial values of each n-th element of the counts vector,
 *  where dT = (tMax-tMin)/counts.size()  */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::histogramData(
    const Types::Core::DateAndTime &tMin, const Types::Core::DateAndTime &tMax,
    std::vector<double> &counts) const {

  size_t nPoints = counts.size();
  if (nPoints == 0)
    return; // nothing to do

  double t0 = static_cast<double>(tMin.totalNanoseconds());
  double t1 = static_cast<double>(tMax.totalNanoseconds());
  if (t0 > t1)
    throw std::invalid_argument(
        "invalid arguments for histogramData; tMax<tMin");

  double dt = (t1 - t0) / static_cast<double>(nPoints);

  for (auto &ev : m_values) {
    double time = static_cast<double>(ev.time().totalNanoseconds());
    if (time < t0 || time >= t1)
      continue;
    size_t ind = static_cast<size_t>((time - t0) / dt);
    counts[ind] += static_cast<double>(ev.value());
  }
}

template <>
void TimeSeriesProperty<std::string>::histogramData(
    const Types::Core::DateAndTime &tMin, const Types::Core::DateAndTime &tMax,
    std::vector<double> &counts) const {
  UNUSED_ARG(tMin);
  UNUSED_ARG(tMax);
  UNUSED_ARG(counts);
  throw std::runtime_error("histogramData is not implememnted for time series "
                           "properties containing strings");
}

/**
 * Get a vector of values taking the filter into account.
 * Values will be excluded if their times lie in a region where the filter is
 * false.
 * @returns :: Vector of included values only
 */
template <typename TYPE>
std::vector<TYPE> TimeSeriesProperty<TYPE>::filteredValuesAsVector() const {
  if (m_filter.empty()) {
    return this->valuesAsVector(); // no filtering to do
  }

  std::vector<TYPE> filteredValues;

  if (!m_filterApplied) {
    applyFilter();
  }

  sortIfNecessary();

  const auto &valueMap = valueAsCorrectMap();
  for (const auto &entry : valueMap) {
    if (isTimeFiltered(entry.first)) {
      filteredValues.push_back(entry.second);
    }
  }

  return filteredValues;
}

/**
 * Find out if the given time is included in the filtered data
 * i.e. it does not lie in an excluded region
 * @param time :: [input] Time to check
 * @returns :: True if time is in an included region, false if the filter
 * excludes it.
 */
template <typename TYPE>
bool TimeSeriesProperty<TYPE>::isTimeFiltered(
    const Types::Core::DateAndTime &time) const {
  if (m_filter.empty()) {
    return false; // no filter
  }

  if (!m_filterApplied) {
    applyFilter();
  }

  // Find which range it lives in
  auto filterEntry = std::lower_bound(
      m_filter.begin(), m_filter.end(), time,
      [](const std::pair<Types::Core::DateAndTime, bool> &filterEntry,
         const Types::Core::DateAndTime &t) { return filterEntry.first < t; });

  if (filterEntry != m_filter.begin()) {
    --filterEntry; // get the latest time BEFORE the given time
  }
  return filterEntry->second;
}

/**
 * Get a list of the splitting intervals, if filtering is enabled.
 * Otherwise the interval is just first time - last time.
 * @returns :: Vector of splitting intervals
 */
template <typename TYPE>
std::vector<SplittingInterval>
TimeSeriesProperty<TYPE>::getSplittingIntervals() const {
  std::vector<SplittingInterval> intervals;
  // Case where there is no filter
  if (m_filter.empty()) {
    intervals.emplace_back(firstTime(), lastTime());
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
        start = firstTime();
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

/// @cond
// -------------------------- Macro to instantiation concrete types
// --------------------------------
#define INSTANTIATE(TYPE) template class TimeSeriesProperty<TYPE>;

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

/// @endcond

} // namespace Kernel
} // namespace Mantid

namespace Mantid {
namespace Kernel {
//================================================================================================
/** Function filtering double TimeSeriesProperties according to the requested
 * statistics.
 *  @param propertyToFilter : Property to filter the statistics on.
 *  @param statisticType : Enum indicating the type of statistics to use.
 *  @return The TimeSeriesProperty filtered by the requested statistics.
 */
double
filterByStatistic(TimeSeriesProperty<double> const *const propertyToFilter,
                  Kernel::Math::StatisticType statisticType) {
  using namespace Kernel::Math;
  double singleValue = 0;
  switch (statisticType) {
  case FirstValue:
    singleValue = propertyToFilter->nthValue(0);
    break;
  case LastValue:
    singleValue = propertyToFilter->nthValue(propertyToFilter->size() - 1);
    break;
  case Minimum:
    singleValue = propertyToFilter->getStatistics().minimum;
    break;
  case Maximum:
    singleValue = propertyToFilter->getStatistics().maximum;
    break;
  case Mean:
    singleValue = propertyToFilter->getStatistics().mean;
    break;
  case Median:
    singleValue = propertyToFilter->getStatistics().median;
    break;
  default:
    throw std::invalid_argument(
        "filterByStatistic - Unknown statistic type: " +
        boost::lexical_cast<std::string>(propertyToFilter));
  };
  return singleValue;
}
} // namespace Kernel
} // namespace Mantid
