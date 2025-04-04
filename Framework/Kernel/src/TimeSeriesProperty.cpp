// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/TimeSeriesProperty.h"
#include "MantidKernel/EmptyValues.h"
#include "MantidKernel/Exception.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/SplittingInterval.h"
#include "MantidKernel/TimeROI.h"
#include "MantidNexus/NeXusFile.hpp"

#include <json/value.h>

#include <boost/regex.hpp>
#include <numeric>

namespace Mantid {
using namespace Types::Core;
namespace Kernel {
namespace {
/// static Logger definition
Logger g_log("TimeSeriesProperty");

/**
 * Check if all values in the input time vector are the same.
 * This assumes there are at least two values.
 * @param values :: a vector of time values.
 * @return :: false if there is at least one non-match, true otherwise.
 */
template <typename TYPE> bool allValuesAreSame(const std::vector<TimeValueUnit<TYPE>> &values) {
  const std::size_t num_values = values.size();
  assert(num_values > 1);
  const auto &first_value = values.front().value();
  for (std::size_t i = 1; i < num_values; ++i) {
    if (first_value != values[i].value())
      return false;
  }
  return true;
}
} // namespace

/**
 * Constructor
 *  @param name :: The name to assign to the property
 */
template <typename TYPE>
TimeSeriesProperty<TYPE>::TimeSeriesProperty(const std::string &name)
    : Property(name, typeid(std::vector<TimeValueUnit<TYPE>>)), m_values(), m_size(), m_propSortedFlag() {}

/**
 * Constructor
 * @param name :: The name to assign to the property
 * @param times :: A vector of DateAndTime objects
 * @param values :: A vector of TYPE
 */
template <typename TYPE>
TimeSeriesProperty<TYPE>::TimeSeriesProperty(const std::string &name,
                                             const std::vector<Types::Core::DateAndTime> &times,
                                             const std::vector<TYPE> &values)
    : TimeSeriesProperty(name) {
  addValues(times, values);
}

/// Virtual destructor
template <typename TYPE> TimeSeriesProperty<TYPE>::~TimeSeriesProperty() = default;

/**
 * "Virtual" copy constructor
 */
template <typename TYPE> TimeSeriesProperty<TYPE> *TimeSeriesProperty<TYPE>::clone() const {
  return new TimeSeriesProperty<TYPE>(*this);
}

/**
 * Construct a TimeSeriesProperty object with the base class data only, no time series data.
 * @param p :: a pointer to a base class object.
 */
template <typename TYPE>
TimeSeriesProperty<TYPE>::TimeSeriesProperty(const Property *const p)
    : Property(*p), m_values(), m_size(), m_propSortedFlag() {}

/**
 * Create a partial copy of this object according to a TimeROI. The partially cloned object
 * should include all time values enclosed by the ROI regions, each defined as [roi_begin,roi_end],
 * plus the values immediately before and after an ROI region, if available.
 * @param timeROI :: time region of interest, i.e. time boundaries used to determine which values should be included in
 * the copy.
 */
template <typename TYPE> Property *TimeSeriesProperty<TYPE>::cloneInTimeROI(const TimeROI &timeROI) const {
  auto filteredTS = new TimeSeriesProperty<TYPE>(this);

  createFilteredData(timeROI, filteredTS->m_values);

  filteredTS->m_size = static_cast<int>(filteredTS->m_values.size());

  return filteredTS;
}

/**
 * "Virutal copy constructor with a time shift
 * @param timeShift :: a time shift in seconds
 */
template <typename TYPE> Property *TimeSeriesProperty<TYPE>::cloneWithTimeShift(const double timeShift) const {
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
template <typename TYPE> std::unique_ptr<TimeSeriesProperty<double>> TimeSeriesProperty<TYPE>::getDerivative() const {

  if (this->m_values.size() < 2) {
    throw std::runtime_error("Derivative is not defined for a time-series "
                             "property with less then two values");
  }

  this->sortIfNecessary();
  auto it = this->m_values.begin();
  int64_t t0 = it->time().totalNanoseconds();
  TYPE v0 = it->value();

  it++;
  auto timeSeriesDeriv = std::make_unique<TimeSeriesProperty<double>>(this->name() + "_derivative");
  timeSeriesDeriv->reserve(this->m_values.size() - 1);
  for (; it != m_values.end(); it++) {
    TYPE v1 = it->value();
    int64_t t1 = it->time().totalNanoseconds();
    if (t1 != t0) {
      double deriv = 1.e+9 * (double(v1 - v0) / double(t1 - t0));
      auto tm = static_cast<int64_t>((t1 + t0) / 2);
      timeSeriesDeriv->addValue(Types::Core::DateAndTime(tm), deriv);
    }
    t0 = t1;
    v0 = v1;
  }
  return timeSeriesDeriv;
}
/** time series derivative specialization for string type */
template <> std::unique_ptr<TimeSeriesProperty<double>> TimeSeriesProperty<std::string>::getDerivative() const {
  throw std::runtime_error("Time series property derivative is not defined for strings");
}

/**
 * Return the memory used by the property, in bytes
 * */
template <typename TYPE> size_t TimeSeriesProperty<TYPE>::getMemorySize() const {
  // Rough estimate
  return m_values.size() * (sizeof(TYPE) + sizeof(DateAndTime));
}

/**
 * Just returns the property (*this) unless overridden
 *  @param rhs a property that is merged in some descendent classes
 *  @return a property with the value
 */
template <typename TYPE> TimeSeriesProperty<TYPE> &TimeSeriesProperty<TYPE>::merge(Property *rhs) {
  return operator+=(rhs);
}

/**
 * Add the value of another property
 * @param right the property to add
 * @return the sum
 */
template <typename TYPE> TimeSeriesProperty<TYPE> &TimeSeriesProperty<TYPE>::operator+=(Property const *right) {
  auto const *rhs = dynamic_cast<TimeSeriesProperty<TYPE> const *>(right);

  if (rhs) {
    if (this->operator!=(*rhs)) {
      m_values.insert(m_values.end(), rhs->m_values.begin(), rhs->m_values.end());
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
template <typename TYPE> bool TimeSeriesProperty<TYPE>::operator==(const TimeSeriesProperty<TYPE> &right) const {
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
template <typename TYPE> bool TimeSeriesProperty<TYPE>::operator==(const Property &right) const {
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
template <typename TYPE> bool TimeSeriesProperty<TYPE>::operator!=(const TimeSeriesProperty<TYPE> &right) const {
  return !(*this == right);
}

/**
 * Deep comparison (not equal).
 * @param right The other property to compare to.
 * @return true if the are not equal.
 */
template <typename TYPE> bool TimeSeriesProperty<TYPE>::operator!=(const Property &right) const {
  return !(*this == right);
}

/**
 * Set name of the property
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::setName(const std::string &name) { m_name = name; }

/**
 * Fill in the supplied vector of time series data according to the input TimeROI. Include all time values
 * within ROI regions, defined as [roi_begin,roi_end], plus the values immediately before and after each ROI region,
 * if available.
 * @param timeROI :: time region of interest, i.e. time boundaries used to determine which values should be included in
 * the filtered data vector
 * @param filteredData :: (output) a vector of TimeValueUnit pairs to be filled in
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::createFilteredData(const TimeROI &timeROI,
                                                  std::vector<TimeValueUnit<TYPE>> &filteredData) const {
  filteredData.clear();

  // Expediently treat a few special cases

  // Nothing to copy
  if (m_values.empty()) {
    return;
  }

  // Copy the only value
  if (m_values.size() == 1) {
    filteredData.push_back(m_values.front());
    return;
  }

  // Copy the first value only, if all values are the same
  // Exclude "proton_charge" logs from consideration, because in a real measurement those values can't be the same,
  // Removing some of them just because they are equal will cause wrong total proton charge results.
  if (allValuesAreSame(m_values) && this->name() != "proton_charge") {
    filteredData.push_back(m_values.front());
    return;
  }

  // Copy everything
  if (timeROI.useAll()) {
    std::copy(m_values.cbegin(), m_values.cend(), std::back_inserter(filteredData));
    return;
  }

  // Copy the first value only
  if (timeROI.useNone()) {
    filteredData.push_back(m_values.front());
    return;
  }

  // Now treat the general case

  // Get all ROI time boundaries. Every other value is start/stop of an ROI "use" region.
  const std::vector<Types::Core::DateAndTime> &roiTimes = timeROI.getAllTimes();
  auto itROI = roiTimes.cbegin();
  const auto itROIEnd = roiTimes.cend();

  auto itValue = m_values.cbegin();
  const auto itValueEnd = m_values.cend();
  auto itLastValueUsed = itValue; // last value used up to the moment

  while (itROI != itROIEnd && itValue != itValueEnd) {
    // Try fast-forwarding the current ROI "use" region towards the current time value. Note, the current value might
    // be in an ROI "ignore" region together with one or more following values.
    while (std::distance(itROI, itROIEnd) > 2 && *(std::next(itROI, 2)) <= itValue->time())
      std::advance(itROI, 2);
    // Try finding the first value equal or past the beginning of the current ROI "use" region.
    itValue = std::lower_bound(itValue, itValueEnd, *itROI,
                               [](const auto &value, const auto &roi_time) { return value.time() < roi_time; });
    // Calculate a [begin,end) range for the values to use
    auto itBeginUseValue = itValue;
    auto itEndUseValue = itValue;
    // If there are no values past the current ROI "use" region, get the previous value
    if (itValue == itValueEnd) {
      itBeginUseValue =
          std::prev(itValue); // std::prev is safe here, because "m_values is empty" case has already been treated above
      itEndUseValue = itValueEnd;
    }
    // If the value is inside the current ROI "use" region, look for other values in the same ROI "use" region
    else if (itValue->time() <= *(std::next(itROI))) {
      // First, try including a value immediately preceding the first value in the ROI "use" region.
      itBeginUseValue = itValue == m_values.begin() ? itValue : std::prev(itValue);
      // Now try finding the first value past the end of the current ROI "use" region.
      while (itValue != itValueEnd && itValue->time() <= *(std::next(itROI)))
        itValue++;
      // Include the current value, therefore, advance itEndUseValue, because std::copy works as [begin,end).
      itEndUseValue = itValue == itValueEnd ? itValue : std::next(itValue);
    }
    // If we are at the last ROI "use" region or the value is not past the beginning of the next ROI "use" region, keep
    // it for the current ROI "use" region.
    else if (std::distance(itROI, itROIEnd) == 2 ||
             (std::distance(itROI, itROIEnd) > 2 && itValue->time() < *(std::next(itROI, 2)))) {
      // Try including the value immediately preceding the current value
      itBeginUseValue = itValue == m_values.begin() ? itValue : std::prev(itValue);
      itEndUseValue = std::next(itValue);
    }
    // Do not use a value already copied for the previous ROI
    if (!filteredData.empty()) {
      itBeginUseValue = std::max(itBeginUseValue, std::next(itLastValueUsed));
    }

    // Copy all [begin,end) values and mark the last value copied
    if (itBeginUseValue < itEndUseValue) {
      std::copy(itBeginUseValue, itEndUseValue, std::back_inserter(filteredData));
      itLastValueUsed = std::prev(itEndUseValue);
    }

    // Move to the next ROI "use" region
    std::advance(itROI, 2);
  }
}

/**
 * Remove time values outside of TimeROI regions each defined as [roi_begin,roi_end].
 * However, keep the values immediately before and after each ROI region, if available.
 * @param timeROI :: a series of time regions used to determine which values to remove or to keep
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::removeDataOutsideTimeROI(const TimeROI &timeROI) {
  std::vector<TimeValueUnit<TYPE>> mp_copy;
  createFilteredData(timeROI, mp_copy);

  m_values.clear();
  m_values = mp_copy;
  mp_copy.clear();

  m_size = static_cast<int>(m_values.size());
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
 * Fill a SplittingIntervalVec that will filter the events by matching
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
void TimeSeriesProperty<TYPE>::makeFilterByValue(std::vector<SplittingInterval> &split, double min, double max,
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
    min = static_cast<double>(minValue());
  if (emptyMax)
    max = static_cast<double>(maxValue());

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
    const DateAndTime lastGoodTime = t;
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
        stop = centre ? lastGoodTime + tol : t;
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
void TimeSeriesProperty<std::string>::makeFilterByValue(std::vector<SplittingInterval> & /*split*/, double /*min*/,
                                                        double /*max*/, double /*TimeTolerance*/,
                                                        bool /*centre*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::makeFilterByValue "
                                       "is not implemented for string "
                                       "properties");
}

/**
 * Fill a TimeROI that will filter the events by matching
 * log values >= min and <= max. Creates TimeROI where
 * times match the log values
 * This method is only used by FilterByLogValue and SumEventsByLogValue
 *
 * @param min :: min value
 * @param max :: max value
 * @param expand :: bool to expand ROI to the expanded Range
 * @param expandRange :: TimeInterval for the expanded ROI
 * @param TimeTolerance :: offset added to times in seconds (default: 0)
 * @param centre :: Whether the log value time is considered centred or at the
 *beginning (the default).
 * @param existingROI :: Optional TimeROI to be intersected with created TimeROI
 * @return The effective TimeROI
 */
template <typename TYPE>
TimeROI TimeSeriesProperty<TYPE>::makeFilterByValue(double min, double max, bool expand,
                                                    const TimeInterval &expandRange, double TimeTolerance, bool centre,
                                                    const TimeROI *existingROI) const {
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
    min = static_cast<double>(minValue());
  if (emptyMax)
    max = static_cast<double>(maxValue());

  TimeROI newROI;

  // Do nothing if the log is empty.
  if (m_values.empty())
    return newROI;

  // 1. Sort
  sortIfNecessary();

  // 2. Do the rest
  const time_duration tol = DateAndTime::durationFromSeconds(TimeTolerance);
  DateAndTime stop_t;
  DateAndTime start, stop;

  bool isGood = false;
  for (size_t i = 0; i < m_values.size(); ++i) {
    TYPE val = m_values[i].value();

    if ((val >= min) && (val <= max)) {
      if (isGood) {
        stop_t = m_values[i].time();
      } else {
        isGood = true;
        stop_t = m_values[i].time();
        start = centre ? m_values[i].time() - tol : m_values[i].time();
      }
    } else if (isGood) {
      stop = centre ? stop_t + tol : m_values[i].time();
      if (start < stop)
        newROI.addROI(start, stop);
      isGood = false;
    }
  }
  if (isGood) {
    stop = centre ? stop_t + tol : stop_t;
    if (start < stop)
      newROI.addROI(start, stop);
  }

  if (expand) {
    if (expandRange.start() < firstTime()) {
      auto val = static_cast<double>(firstValue());
      if ((val >= min) && (val <= max)) {
        newROI.addROI(expandRange.start(), firstTime());
      }
    }
    if (expandRange.stop() > lastTime()) {
      auto val = static_cast<double>(lastValue());
      if ((val >= min) && (val <= max)) {
        newROI.addROI(lastTime(), expandRange.stop());
      }
    }
  }

  // If the TimeROI is empty there are no values inside the filter
  // so we should return USE_NONE
  if (newROI.useAll()) {
    return TimeROI::USE_NONE;
  }

  if (existingROI != nullptr && !existingROI->useAll()) {
    newROI.update_intersection(*existingROI);
  }
  return newROI;
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
TimeROI TimeSeriesProperty<std::string>::makeFilterByValue(double /*min*/, double /*max*/, bool /*expand*/,
                                                           const TimeInterval & /*expandRange*/,
                                                           double /*TimeTolerance*/, bool /*centre*/,
                                                           const TimeROI * /*existingROI*/) const {
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
void TimeSeriesProperty<TYPE>::expandFilterToRange(std::vector<SplittingInterval> &split, double min, double max,
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
    min = static_cast<double>(minValue());
  if (emptyMax)
    max = static_cast<double>(maxValue());

  if (range.start() < firstTime()) {
    // Assume everything before the 1st value is constant
    double val = static_cast<double>(firstValue());
    if ((val >= min) && (val <= max)) {
      SplittingIntervalVec extraFilter;
      extraFilter.emplace_back(range.start(), firstTime(), 0);
      // Include everything from the start of the run to the first time measured
      // (which may be a null time interval; this'll be ignored)
      split = split | extraFilter;
    }
  }

  if (lastTime() < range.stop()) {
    // Assume everything after the LAST value is constant
    double val = static_cast<double>(lastValue());
    if ((val >= min) && (val <= max)) {
      SplittingIntervalVec extraFilter;
      extraFilter.emplace_back(lastTime(), range.stop(), 0);
      // Include everything from the start of the run to the first time measured
      // (which may be a null time interval; this'll be ignored)
      split = split | extraFilter;
    }
  }
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
void TimeSeriesProperty<std::string>::expandFilterToRange(std::vector<SplittingInterval> & /*split*/, double /*min*/,
                                                          double /*max*/, const TimeInterval & /*range*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::makeFilterByValue "
                                       "is not implemented for string "
                                       "properties");
}

/** Returns the calculated time weighted average value.
 * @param timeRoi  Object that holds information about when the time measurement was active.
 * @return The time-weighted average value of the log when the time measurement was active.
 */
template <typename TYPE> double TimeSeriesProperty<TYPE>::timeAverageValue(const TimeROI *timeRoi) const {
  double retVal = 0.0;
  try {
    if ((timeRoi == nullptr) || (timeRoi->useAll())) {
      const auto &intervals = getTimeIntervals();
      retVal = this->averageValueInFilter(intervals);
    } else if (timeRoi->useNone()) {
      // if TimeROI bans everything, use the simple mean
      const auto stats =
          Mantid::Kernel::getStatistics(this->valuesAsVector(), Mantid::Kernel::Math::StatisticType::Mean);
      return stats.mean;
    } else {
      const auto &filter = timeRoi->toTimeIntervals();
      retVal = this->averageValueInFilter(filter);
      g_log.warning("Calls to TimeSeriesProperty::timeAverageValue should be replaced with "
                    "Run::getTimeAveragedValue");
    }
  } catch (std::exception &) {
    // just return nan
    retVal = std::numeric_limits<double>::quiet_NaN();
  }
  return retVal;
}

template <> double TimeSeriesProperty<std::string>::timeAverageValue(const TimeROI * /*timeRoi*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::timeAverageValue is not implemented for string properties");
}

/** Calculates the time-weighted average of a property in a filtered range.
 *  This is written for that case of logs whose values start at the times given.
 *  @param filter The splitter/filter restricting the range of values included
 *  @return The time-weighted average value of the log in the range within the
 * filter.
 */
template <typename TYPE>
double TimeSeriesProperty<TYPE>::averageValueInFilter(const std::vector<TimeInterval> &filter) const {
  // TODO: Consider logs that aren't giving starting values.

  // If there's just a single value in the log, return that.
  if (size() == 1) {
    return static_cast<double>(this->firstValue());
  }

  // First of all, if the log or the filter is empty, return NaN
  if (realSize() == 0 || filter.empty()) {
    return std::numeric_limits<double>::quiet_NaN();
  }

  sortIfNecessary();

  double numerator(0.0), totalTime(0.0);
  // Loop through the filter ranges
  for (const auto &time : filter) {
    // Calculate the total time duration (in seconds) within by the filter
    totalTime += time.duration();

    // Get the log value and index at the start time of the filter
    int index;
    double currentValue = static_cast<double>(getSingleValue(time.start(), index));
    DateAndTime startTime = time.start();

    while (index < realSize() - 1 && m_values[index + 1].time() < time.stop()) {
      ++index;
      numerator += DateAndTime::secondsFromDuration(m_values[index].time() - startTime) * currentValue;
      startTime = m_values[index].time();
      currentValue = static_cast<double>(m_values[index].value());
    }

    // Now close off with the end of the current filter range
    numerator += DateAndTime::secondsFromDuration(time.stop() - startTime) * currentValue;
  }

  if (totalTime > 0) {
    // 'Normalise' by the total time
    return numerator / totalTime;
  } else {
    // give simple mean
    const auto stats = Mantid::Kernel::getStatistics(this->valuesAsVector(), Mantid::Kernel::Math::StatisticType::Mean);
    return stats.mean;
  }
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
double TimeSeriesProperty<std::string>::averageValueInFilter(const std::vector<TimeInterval> & /*filter*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::"
                                       "averageValueInFilter is not "
                                       "implemented for string properties");
}

template <typename TYPE>
std::pair<double, double>
TimeSeriesProperty<TYPE>::averageAndStdDevInFilter(const std::vector<TimeInterval> &intervals) const {
  double mean_prev, mean_current(0.0), s(0.0), variance, duration, weighted_sum(0.0);

  // First of all, if the log or the intervals are empty or is a single value,
  // return NaN for the uncertainty
  if (realSize() <= 1 || intervals.empty()) {
    return std::pair<double, double>{this->averageValueInFilter(intervals), std::numeric_limits<double>::quiet_NaN()};
  }
  auto real_size = realSize();
  for (const auto &time : intervals) {
    int index;
    auto currentValue = static_cast<double>(getSingleValue(time.start(), index));
    DateAndTime startTime = time.start();
    while (index < realSize() - 1 && m_values[index + 1].time() < time.stop()) {
      index++;
      if (index == real_size) {
        duration = DateAndTime::secondsFromDuration(time.stop() - startTime);
      } else {
        duration = DateAndTime::secondsFromDuration(m_values[index].time() - startTime);
        startTime = m_values[index].time();
      }
      mean_prev = mean_current;
      if (duration > 0.) {
        weighted_sum += duration;

        mean_current = mean_prev + (duration / weighted_sum) * (currentValue - mean_prev);
        s += duration * (currentValue - mean_prev) * (currentValue - mean_current);
      }
      currentValue = static_cast<double>(m_values[index].value());
    }

    // Now close off with the end of the current filter range
    duration = DateAndTime::secondsFromDuration(time.stop() - startTime);
    if (duration > 0.) {
      weighted_sum += duration;
      mean_prev = mean_current;

      mean_current = mean_prev + (duration / weighted_sum) * (currentValue - mean_prev);
      s += duration * (currentValue - mean_prev) * (currentValue - mean_current);
    }
  }
  variance = s / weighted_sum;
  // Normalise by the total time
  return std::pair<double, double>{mean_current, std::sqrt(variance)};
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
std::pair<double, double>
TimeSeriesProperty<std::string>::averageAndStdDevInFilter(const std::vector<TimeInterval> & /*filter*/) const {
  throw Exception::NotImplementedError("TimeSeriesProperty::"
                                       "averageAndStdDevInFilter is not "
                                       "implemented for string properties");
}

template <typename TYPE>
std::pair<double, double> TimeSeriesProperty<TYPE>::timeAverageValueAndStdDev(const Kernel::TimeROI *timeRoi) const {
  // time series with less than two entries are conner cases
  if (this->realSize() == 0)
    return std::pair<double, double>{std::numeric_limits<double>::quiet_NaN(),
                                     std::numeric_limits<double>::quiet_NaN()};
  else if (this->realSize() == 1)
    return std::pair<double, double>(static_cast<double>(this->firstValue()), 0.0);

  // Derive splitting intervals from either the roi or from the first/last entries in the time series
  std::vector<TimeInterval> intervals;
  if (timeRoi && !timeRoi->useAll()) {
    intervals = timeRoi->toTimeIntervals(this->firstTime());
  } else {
    intervals = this->getTimeIntervals();
  }

  return this->averageAndStdDevInFilter(intervals);
}

/** Function specialization for timeAverageValueAndStdDev<std::string>
 *  @return pair (Nan, Nan) always.
 */
template <>
std::pair<double, double>
TimeSeriesProperty<std::string>::timeAverageValueAndStdDev(const Kernel::TimeROI * /*roi*/) const {
  throw Exception::NotImplementedError(
      "TimeSeriesProperty::timeAverageValueAndStdDev is not implemented for string properties");
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
template <typename TYPE> std::map<DateAndTime, TYPE> TimeSeriesProperty<TYPE>::valueAsCorrectMap() const {
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
template <typename TYPE> std::vector<TYPE> TimeSeriesProperty<TYPE>::valuesAsVector() const {
  sortIfNecessary();

  std::vector<TYPE> out;
  out.reserve(m_values.size());

  for (size_t i = 0; i < m_values.size(); i++)
    out.emplace_back(m_values[i].value());

  return out;
}

/**
 * Return the time series as a C++ multimap<DateAndTime, TYPE>. All values.
 * This method is used in parsing the ISIS ICPevent log file: different
 * commands
 * can be recorded against the same time stamp but all must be present.
 */
template <typename TYPE> std::multimap<DateAndTime, TYPE> TimeSeriesProperty<TYPE>::valueAsMultiMap() const {
  std::multimap<DateAndTime, TYPE> asMultiMap;

  if (!m_values.empty()) {
    for (size_t i = 0; i < m_values.size(); i++)
      asMultiMap.insert(std::make_pair(m_values[i].time(), m_values[i].value()));
  }

  return asMultiMap;
}

/**
 * Return the time series's times as a vector<DateAndTime>
 * @return A vector of DateAndTime objects
 */
template <typename TYPE> std::vector<DateAndTime> TimeSeriesProperty<TYPE>::timesAsVector() const {
  sortIfNecessary();

  std::vector<DateAndTime> out;
  out.reserve(m_values.size());

  for (size_t i = 0; i < m_values.size(); i++) {
    out.emplace_back(m_values[i].time());
  }

  return out;
}

/**
 * A view of the times as seen within a the TimeROI's regions.
 * A values will be excluded if its times lie outside the ROI's regions or coincides with the upper boundary
 * of a ROI region. For instance, time "2007-11-30T16:17:30" is excluded in the ROI
 * ["2007-11-30T16:17:00", "2007-11-30T16:17:30"). However, a value at time "2007-11-30T16:16:00" will be included
 * with a time of "2007-11-30T16:17:00" because that is when the ROI starts.
 * @param roi :: ROI regions validating any query time.
 * @return A vector of DateAndTime objects
 */
template <typename TYPE>
std::vector<DateAndTime> TimeSeriesProperty<TYPE>::filteredTimesAsVector(const Kernel::TimeROI *roi) const {
  if (roi && !roi->useAll()) {
    this->sortIfNecessary();
    std::vector<DateAndTime> filteredTimes;
    if (roi->firstTime() > this->m_values.back().time()) {
      // Since the ROI starts after everything, just return the last time in the log
      filteredTimes.emplace_back(roi->firstTime());
    } else { // only use the times in the filter - this is very similar to FilteredTimeSeriesProperty::applyFilter
      // the index into the m_values array of the time, or -1 (before) or m_values.size() (after)
      std::size_t index_current_log{0};

      for (const auto &splitter : roi->toTimeIntervals()) {
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

          // start time is when this value was created or when the filter started
          filteredTimes.emplace_back(std::max(beginTime, this->m_values[index_current_log].time()));
        }
        // go back one so the next splitter can add a value
        if (index_current_log > 0)
          index_current_log--;
      }
    }

    return filteredTimes;
  } else {
    return this->timesAsVector();
  }
}

template <typename TYPE> std::vector<DateAndTime> TimeSeriesProperty<TYPE>::filteredTimesAsVector() const {
  return this->timesAsVector();
}

/**
 * @return Return the series as list of times, where the time is the number of
 * seconds since the start.
 */
template <typename TYPE> std::vector<double> TimeSeriesProperty<TYPE>::timesAsVectorSeconds() const {
  // 1. Sort if necessary
  sortIfNecessary();

  // 2. Output data structure
  std::vector<double> out;
  out.reserve(m_values.size());

  Types::Core::DateAndTime start = m_values[0].time();
  for (size_t i = 0; i < m_values.size(); i++) {
    out.emplace_back(DateAndTime::secondsFromDuration(m_values[i].time() - start));
  }

  return out;
}

/** Add a value to the series.
 *  Added values need not be sequential in time.
 *  @param time   The time
 *  @param value  The associated value
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::addValue(const Types::Core::DateAndTime &time, const TYPE &value) {
  TimeValueUnit<TYPE> newvalue(time, value);
  // Add the value to the back of the vector
  m_values.emplace_back(newvalue);
  // Increment the separate record of the property's size
  m_size++;

  // Toggle the sorted flag if necessary
  // (i.e. if the flag says we're sorted and the added time is before the prior
  // last time)
  if (m_size == 1) {
    // First item, must be sorted.
    m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
  } else if (m_propSortedFlag == TimeSeriesSortStatus::TSUNKNOWN && m_values.back() < *(m_values.rbegin() + 1)) {
    // Previously unknown and still unknown
    m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
  } else if (m_propSortedFlag == TimeSeriesSortStatus::TSSORTED && m_values.back() < *(m_values.rbegin() + 1)) {
    // Previously sorted but last added is not in order
    m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
  }

  // m_filterApplied = false;
}

/** Add a value to the map
 *  @param time :: The time as a string in the format: (ISO 8601)
 * yyyy-mm-ddThh:mm:ss
 *  @param value :: The associated value
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::addValue(const std::string &time, const TYPE &value) {
  return addValue(Types::Core::DateAndTime(time), value);
}

/**
 * Add a value to the map using a time_t
 *  @param time :: The time as a time_t value
 *  @param value :: The associated value
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::addValue(const std::time_t &time, const TYPE &value) {
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
void TimeSeriesProperty<TYPE>::addValues(const std::vector<Types::Core::DateAndTime> &times,
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
void TimeSeriesProperty<TYPE>::replaceValues(const std::vector<Types::Core::DateAndTime> &times,
                                             const std::vector<TYPE> &values) {
  clear();
  addValues(times, values);
}

/**
 * Returns the last time
 * @return Value
 */
template <typename TYPE> DateAndTime TimeSeriesProperty<TYPE>::lastTime() const {
  if (m_values.empty()) {
    const std::string error("lastTime(): TimeSeriesProperty '" + name() + "' is empty");
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
    const std::string error("firstValue(): TimeSeriesProperty '" + name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  sortIfNecessary();

  return m_values[0].value();
}

template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::firstValue(const Kernel::TimeROI &roi) const {
  const auto startTime = roi.firstTime();
  if (startTime <= this->firstTime()) {
    return this->firstValue();
  } else if (startTime >= this->lastTime()) {
    return this->lastValue();
  } else {
    const auto times = this->timesAsVector();
    auto iter = std::lower_bound(times.cbegin(), times.cend(), startTime);
    if (*iter > startTime)
      iter--;
    const auto index = std::size_t(std::distance(times.cbegin(), iter));

    const auto values = this->valuesAsVector();
    const TYPE ret = values[index];
    return ret;
  }
}

/** Returns the first time regardless of filter
 *  @return Value
 */
template <typename TYPE> DateAndTime TimeSeriesProperty<TYPE>::firstTime() const {
  if (m_values.empty()) {
    const std::string error("firstTime(): TimeSeriesProperty '" + name() + "' is empty");
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
    const std::string error("lastValue(): TimeSeriesProperty '" + name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  sortIfNecessary();

  return m_values.rbegin()->value();
}

template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::lastValue(const Kernel::TimeROI &roi) const {
  const auto stopTime = roi.lastTime();
  const auto times = this->timesAsVector();
  if (stopTime <= times.front()) {
    return this->firstValue();
  } else if (stopTime >= times.back()) {
    return this->lastValue();
  } else {
    auto iter = std::lower_bound(times.cbegin(), times.cend(), stopTime);
    if ((iter != times.cbegin()) && (*iter > stopTime))
      --iter;
    const auto index = std::size_t(std::distance(times.cbegin(), iter));

    const auto values = this->valuesAsVector();
    const TYPE ret = values[index];
    return ret;
  }
}

/**
 * Returns duration of the time series, possibly restricted by a TimeROI object.
 * If no TimeROI is provided or the TimeROI is empty, the whole span of the time series plus
 * and additional extra time is returned. This extra time is the time span between the last two log entries.
 * The extra time ensures that the mean and the time-weighted mean are the same for time series
 * containing log entries equally spaced in time.
 * @param roi :: TimeROI object defining the time segments to consider.
 * @return duration, in seconds.
 */
template <typename TYPE> double TimeSeriesProperty<TYPE>::durationInSeconds(const Kernel::TimeROI *roi) const {
  if (this->size() == 0)
    return std::numeric_limits<double>::quiet_NaN();
  if (roi && !roi->useAll()) {
    Kernel::TimeROI seriesSpan(*roi);
    const auto thisFirstTime = this->firstTime();
    // remove everything before the start time
    if (thisFirstTime > DateAndTime::GPS_EPOCH) {
      seriesSpan.addMask(DateAndTime::GPS_EPOCH, thisFirstTime);
    }
    return seriesSpan.durationInSeconds();
  } else {
    const auto &intervals = this->getTimeIntervals();
    const double duration_sec =
        std::accumulate(intervals.cbegin(), intervals.cend(), 0.,
                        [](double sum, const auto &interval) { return sum + interval.duration(); });
    return duration_sec;
  }
}

template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::minValue() const {
  return std::min_element(m_values.begin(), m_values.end(), TimeValueUnit<TYPE>::valueCmp)->value();
}

template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::maxValue() const {
  return std::max_element(m_values.begin(), m_values.end(), TimeValueUnit<TYPE>::valueCmp)->value();
}

template <typename TYPE> double TimeSeriesProperty<TYPE>::mean() const {
  Mantid::Kernel::Statistics raw_stats =
      Mantid::Kernel::getStatistics(this->filteredValuesAsVector(), StatOptions::Mean);
  return raw_stats.mean;
}

/// Returns the number of values at UNIQUE time intervals in the time series
/// @returns The number of unique time interfaces
template <typename TYPE> int TimeSeriesProperty<TYPE>::size() const { return m_size; }

/**
 * Returns the real size of the time series property map:
 * the number of entries, including repeated ones.
 */
template <typename TYPE> int TimeSeriesProperty<TYPE>::realSize() const { return static_cast<int>(m_values.size()); }

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
template <typename TYPE> std::vector<std::string> TimeSeriesProperty<TYPE>::time_tValue() const {
  sortIfNecessary();

  std::vector<std::string> values;
  values.reserve(m_values.size());

  for (size_t i = 0; i < m_values.size(); i++) {
    std::stringstream line;
    line << m_values[i].time().toSimpleString() << " " << m_values[i].value();
    values.emplace_back(line.str());
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
template <typename TYPE> std::map<DateAndTime, TYPE> TimeSeriesProperty<TYPE>::valueAsMap() const {
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
template <typename TYPE> std::string TimeSeriesProperty<TYPE>::setValue(const std::string & /*unused*/) {
  throw Exception::NotImplementedError("TimeSeriesProperty<TYPE>::setValue - "
                                       "Cannot extract TimeSeries from a "
                                       "std::string");
}

/**
 * Set the property from a Json value. Throws a NotImplementedError
 *  @throw Exception::NotImplementedError Not yet implemented
 * @return Nothing in this case
 */
template <typename TYPE> std::string TimeSeriesProperty<TYPE>::setValueFromJson(const Json::Value & /*unused*/) {
  throw Exception::NotImplementedError("TimeSeriesProperty<TYPE>::setValue - "
                                       "Cannot extract TimeSeries from a "
                                       "Json::Value");
}

/**
 * @throw Exception::NotImplementedError Not yet implemented
 * @return Nothing in this case
 */
template <typename TYPE>
std::string TimeSeriesProperty<TYPE>::setDataItem(const std::shared_ptr<DataItem> & /*unused*/) {
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
  // m_filterApplied = false;
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
    auto lastValueInVec = m_values.back();
    clear();
    m_values.emplace_back(lastValueInVec);
    m_size = 1;
  }
}

//--------------------------------------------------------------------------------------------
/**
 * Clears and creates a TimeSeriesProperty from the parameters. It is extremely similar to the other TSP::create with
 * specialized conversion.
 *  @param start_time :: The reference time as a boost::posix_time::ptime value
 *  @param time_sec :: A vector of time offset (from start_time) in seconds.
 *  @param new_values :: A vector of values, each corresponding to the time
 * offset in time_sec.
 *    Vector sizes must match.
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::create(const Types::Core::DateAndTime &start_time, const std::vector<double> &time_sec,
                                      const std::vector<TYPE> &new_values) {
  if (time_sec.size() != new_values.size()) {
    std::stringstream msg;
    msg << "TimeSeriesProperty \"" << name() << "\" create: mismatched size "
        << "for the time and values vectors.";
    throw std::invalid_argument(msg.str());
  }

  clear();
  const std::size_t num = new_values.size();
  m_values.reserve(num);

  // set the sorted flag
  if (std::is_sorted(time_sec.cbegin(), time_sec.cend()))
    m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
  else
    m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
  // set the values
  constexpr double SEC_TO_NANO{1000000000.0};
  const int64_t start_time_ns = start_time.totalNanoseconds();
  for (std::size_t i = 0; i < num; i++) {
    m_values.emplace_back(start_time_ns + static_cast<int64_t>(time_sec[i] * SEC_TO_NANO), new_values[i]);
  }

  // reset the size
  m_size = static_cast<int>(m_values.size());
}

//--------------------------------------------------------------------------------------------
/** Clears and creates a TimeSeriesProperty from the parameters. It is extremely similar to the other TSP::create with
 *specialized conversion.
 *
 * @param new_times :: A vector of DateAndTime.
 * @param new_values :: A vector of values, each corresponding to the time off
 *set in new_time.
 *                      Vector sizes must match.
 */
template <typename TYPE>
void TimeSeriesProperty<TYPE>::create(const std::vector<Types::Core::DateAndTime> &new_times,
                                      const std::vector<TYPE> &new_values) {
  if (new_times.size() != new_values.size())
    throw std::invalid_argument("TimeSeriesProperty::create: mismatched size "
                                "for the time and values vectors.");

  clear();
  // nothing to do without values
  if (new_times.empty()) {
    m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
    return;
  }

  const std::size_t num = new_values.size();
  m_values.reserve(num);

  // set the sorted flag
  if (std::is_sorted(new_times.cbegin(), new_times.cend()))
    m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
  else
    m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
  // add the values
  for (std::size_t i = 0; i < num; i++) {
    m_values.emplace_back(new_times[i], new_values[i]);
  }

  // reset the size
  m_size = static_cast<int>(m_values.size());
}

/** Returns the value at a particular time
 *  @param t :: time
 *  @return Value at time \a t
 */
template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::getSingleValue(const Types::Core::DateAndTime &t) const {
  if (m_values.empty()) {
    const std::string error("getSingleValue(): TimeSeriesProperty '" + name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 1. Get sorted
  sortIfNecessary();

  // 2.
  TYPE valueAtTime;
  if (t < m_values[0].time()) {
    // 1. Out side of lower bound
    valueAtTime = m_values[0].value();
  } else if (t >= m_values.back().time()) {
    // 2. Out side of upper bound
    valueAtTime = m_values.back().value();
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
      errss << "TimeSeriesProperty.findIndex() returns index (" << index << " ) > maximum defined value "
            << m_values.size();
      throw std::logic_error(errss.str());
    }

    valueAtTime = m_values[static_cast<size_t>(index)].value();
  }

  return valueAtTime;
} // END-DEF getSinglevalue()

/** Returns the value at a particular time
 *  @param t :: time
 *  @param index :: index of time
 *  @return Value at time \a t
 */
template <typename TYPE>
TYPE TimeSeriesProperty<TYPE>::getSingleValue(const Types::Core::DateAndTime &t, int &index) const {
  if (m_values.empty()) {
    const std::string error("getSingleValue(): TimeSeriesProperty '" + name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 1. Get sorted
  sortIfNecessary();

  // 2.
  TYPE valueAtTime;
  if (t < m_values[0].time()) {
    // 1. Out side of lower bound
    valueAtTime = m_values[0].value();
    index = 0;
  } else if (t >= m_values.back().time()) {
    // 2. Out side of upper bound
    valueAtTime = m_values.back().value();
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
      errss << "TimeSeriesProperty.findIndex() returns index (" << index << " ) > maximum defined value "
            << m_values.size();
      throw std::logic_error(errss.str());
    }

    valueAtTime = m_values[static_cast<size_t>(index)].value();
  }

  return valueAtTime;
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
template <typename TYPE> TimeInterval TimeSeriesProperty<TYPE>::nthInterval(int n) const {
  // 0. Throw exception
  if (m_values.empty()) {
    const std::string error("nthInterval(): TimeSeriesProperty '" + name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 1. Sort
  sortIfNecessary();

  // 2. Calculate time interval

  Kernel::TimeInterval deltaT;

  // No filter
  if (n >= static_cast<int>(m_values.size()) || (n == static_cast<int>(m_values.size()) - 1 && m_values.size() == 1)) {
    // Out of bound
    ;
  } else if (n == static_cast<int>(m_values.size()) - 1) {
    // Last one by making up an end time.
    DateAndTime endTime = getFakeEndTime();

    deltaT = Kernel::TimeInterval(m_values.rbegin()->time(), endTime);
  } else {
    // Regular
    DateAndTime startT = m_values[static_cast<std::size_t>(n)].time();
    DateAndTime endT = m_values[static_cast<std::size_t>(n) + 1].time();
    TimeInterval dt(startT, endT);
    deltaT = dt;
  }

  return deltaT;
}

template <typename TYPE> Types::Core::DateAndTime TimeSeriesProperty<TYPE>::getFakeEndTime() const {
  sortIfNecessary();

  // the last time is the last thing known
  const auto ultimate = m_values.rbegin()->time();

  // go backwards from the time before it that is different
  int counter = 0;
  while (DateAndTime::secondsFromDuration(ultimate - (m_values.rbegin() + counter)->time()) == 0.) {
    counter += 1;
  }

  // get the last time that is different
  time_duration lastDuration = m_values.rbegin()->time() - (m_values.rbegin() + counter)->time();

  // the last duration is equal to the previous, non-zero, duration
  return m_values.rbegin()->time() + lastDuration;
}

//-----------------------------------------------------------------------------------------------
/** Returns n-th value of n-th interval in an incredibly inefficient way.
 *  The algorithm is migrated from mthInterval()
 *  @param n :: index
 *  @return Value
 */
template <typename TYPE> TYPE TimeSeriesProperty<TYPE>::nthValue(int n) const {

  // 1. Throw error if property is empty
  if (m_values.empty()) {
    const std::string error("nthValue(): TimeSeriesProperty '" + name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  // 2. Sort and apply filter
  sortIfNecessary();

  TYPE nthValue;

  // 3. Situation 1:  No filter
  if (static_cast<size_t>(n) < m_values.size()) {
    const auto entry = m_values[static_cast<std::size_t>(n)];
    nthValue = entry.value();
  } else {
    const auto entry = m_values[static_cast<std::size_t>(m_size) - 1];
    nthValue = entry.value();
  }

  return nthValue;
}

/** Returns n-th time, or the last time if fewer than n entries.
 *  Special cases: There is no special cases
 *  @param n :: index
 *  @return DateAndTime
 */
template <typename TYPE> Types::Core::DateAndTime TimeSeriesProperty<TYPE>::nthTime(int n) const {
  sortIfNecessary();

  if (m_values.empty()) {
    const std::string error("nthTime(): TimeSeriesProperty '" + name() + "' is empty");
    g_log.debug(error);
    throw std::runtime_error(error);
  }

  if (n < 0 || n >= static_cast<int>(m_values.size()))
    n = static_cast<int>(m_values.size()) - 1;

  return m_values[static_cast<size_t>(n)].time();
}

/**
 * Updates size()
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::countSize() const {
  // 1. Not filter
  m_size = int(m_values.size());
}

/**  Check if str has the right time format
 *   @param str :: The string to check
 *   @return True if the format is correct, false otherwise.
 */
template <typename TYPE> bool TimeSeriesProperty<TYPE>::isTimeString(const std::string &str) {
  static const boost::regex re("^[0-9]{4}.[0-9]{2}.[0-9]{2}.[0-9]{2}.[0-9]{2}.[0-9]{2}");
  return boost::regex_search(str.begin(), str.end(), re);
}

/**
 * This doesn't check anything -we assume these are always valid
 *  @returns an empty string ""
 */
template <typename TYPE> std::string TimeSeriesProperty<TYPE>::isValid() const { return ""; }

/*
 * A TimeSeriesProperty never has a default, so return empty string
 * @returns Empty string as no defaults can be provided
 */
template <typename TYPE> std::string TimeSeriesProperty<TYPE>::getDefault() const {
  return ""; // No defaults can be provided=empty string
}

/**
 * A TimeSeriesProperty never has a default
 */
template <typename TYPE> bool TimeSeriesProperty<TYPE>::isDefault() const { return false; }

/**
 * Return a TimeSeriesPropertyStatistics struct containing the
 * statistics of this TimeSeriesProperty object.
 * @param roi : Optional TimeROI pointer to get statistics for active time.
 *
 * N.B. This method DOES take filtering into account
 */
template <typename TYPE>
TimeSeriesPropertyStatistics TimeSeriesProperty<TYPE>::getStatistics(const TimeROI *roi) const {
  // Start with statistics that are not time-weighted
  TimeSeriesPropertyStatistics out(Mantid::Kernel::getStatistics(this->filteredValuesAsVector(roi)));
  out.duration = this->durationInSeconds(roi);
  if (out.standard_deviation == 0.) {
    // if the simple std-dev is zero, just copy the simple mean
    out.time_mean = out.mean;
    out.time_standard_deviation = 0.;
  } else {
    // follow with time-weighted statistics
    auto avAndDev = this->timeAverageValueAndStdDev(roi);
    out.time_mean = avAndDev.first;
    out.time_standard_deviation = avAndDev.second;
  }
  return out;
}

template <>
TimeSeriesPropertyStatistics TimeSeriesProperty<std::string>::getStatistics(const TimeROI * /* roi*/) const {
  // statistics of a string property doesn't make sense
  TimeSeriesPropertyStatistics out;
  out.setAllToNan();

  return out;
}

/** Calculate a particular statistical quantity from the values of the time series.
 *  @param selection : Enum indicating the selected statistical quantity.
 *  @param roi : optional pointer to TimeROI object for filtering the time series values.
 *  @return The value of the computed statistical quantity.
 */
template <typename TYPE>
double TimeSeriesProperty<TYPE>::extractStatistic(Math::StatisticType selection, const TimeROI *roi) const {
  using namespace Kernel::Math;
  double singleValue = 0;
  switch (selection) {
  case FirstValue:
    if (roi && !roi->useAll())
      singleValue = double(this->firstValue(*roi));
    else
      singleValue = double(this->nthValue(0));
    break;
  case LastValue:
    if (roi && !roi->useAll())
      singleValue = double(this->lastValue(*roi));
    else
      singleValue = double(this->nthValue(this->size() - 1));
    break;
  case Minimum:
    singleValue = static_cast<double>(this->getStatistics(roi).minimum);
    break;
  case Maximum:
    singleValue = static_cast<double>(this->getStatistics(roi).maximum);
    break;
  case Mean:
    singleValue = this->getStatistics(roi).mean;
    break;
  case Median:
    singleValue = this->getStatistics(roi).median;
    break;
  case TimeAveragedMean:
    singleValue = this->getStatistics(roi).time_mean;
    break;
  case StdDev:
    singleValue = this->getStatistics(roi).standard_deviation;
    break;
  case TimeAverageStdDev:
    singleValue = this->getStatistics(roi).time_standard_deviation;
    break;
  default:
    throw std::invalid_argument("extractStatistic - Unknown statistic type: " + boost::lexical_cast<std::string>(this));
  };
  return singleValue;
}

/** Function specialization for TimeSeriesProperty<std::string>
 *  @throws Kernel::Exception::NotImplementedError always
 */
template <>
double TimeSeriesProperty<std::string>::extractStatistic(Math::StatisticType selection, const TimeROI *roi) const {
  UNUSED_ARG(selection);
  UNUSED_ARG(roi);
  throw Exception::NotImplementedError("TimeSeriesProperty::"
                                       "extractStatistic is not "
                                       "implemented for string properties");
}

/*
 * Detects whether there are duplicated entries (of time) in property
 * If there is any, keep one of them
 */
template <typename TYPE> void TimeSeriesProperty<TYPE>::eliminateDuplicates() {
  // ensure that the values are sorted
  sortIfNecessary();

  // cache the original size so the number removed can be reported
  const auto origSize{m_size};

  // remove the first n-repeats
  // taken from
  // https://stackoverflow.com/questions/21060636/using-stdunique-and-vector-erase-to-remove-all-but-last-occurrence-of-duplicat
  auto it = std::unique(m_values.rbegin(), m_values.rend(),
                        [](const auto &a, const auto &b) { return a.time() == b.time(); });
  m_values.erase(m_values.begin(), it.base());

  // update m_size
  countSize();

  // log how many values were removed
  const auto numremoved = origSize - m_size;
  if (numremoved > 0)
    g_log.notice() << "Log \"" << this->name() << "\" has " << numremoved << " entries removed due to duplicated time. "
                   << "\n";
}

/*
 * Print the content to string
 */
template <typename TYPE> std::string TimeSeriesProperty<TYPE>::toString() const {
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
template <typename TYPE> void TimeSeriesProperty<TYPE>::sortIfNecessary() const {
  if (m_propSortedFlag == TimeSeriesSortStatus::TSUNKNOWN) {
    bool sorted = is_sorted(m_values.begin(), m_values.end());
    if (sorted)
      m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
    else
      m_propSortedFlag = TimeSeriesSortStatus::TSUNSORTED;
  }

  if (m_propSortedFlag == TimeSeriesSortStatus::TSUNSORTED) {
    g_log.information() << "TimeSeriesProperty \"" << this->name()
                        << "\" is not sorted.  Sorting is operated on it. \n";
    std::stable_sort(m_values.begin(), m_values.end());
    m_propSortedFlag = TimeSeriesSortStatus::TSSORTED;
  }
}

/** Find the index of the entry of time t in the mP vector (sorted)
 *  Return @ if t is within log.begin and log.end, then the index of the log
 *  equal or just smaller than t
 *           if t is earlier (less) than the starting time, return -1
 *           if t is later (larger) than the ending time, return m_value.size
 */
template <typename TYPE> int TimeSeriesProperty<TYPE>::findIndex(Types::Core::DateAndTime t) const {
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
int TimeSeriesProperty<TYPE>::upperBound(Types::Core::DateAndTime t, int istart, int iend) const {
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
  const TimeValueUnit<TYPE> temppair(t, m_values[0].value());
  const auto first = m_values.cbegin() + istart;
  const auto last = m_values.cbegin() + iend + 1;
  const auto iter = std::lower_bound(first, last, temppair);

  // 4. Calculate return value
  if (iter == last)
    throw std::runtime_error("Cannot find data");
  return static_cast<int>(std::distance(m_values.cbegin(), iter));
}

/**
 * Set the value of the property via a reference to another property.
 * If the value is unacceptable the value is not changed but a string is
 * returned.
 * The value is only accepted if the other property has the same type as this
 * @param right :: A reference to a property.
 */
template <typename TYPE> std::string TimeSeriesProperty<TYPE>::setValueFromProperty(const Property &right) {
  auto prop = dynamic_cast<const TimeSeriesProperty<TYPE> *>(&right);
  if (!prop) {
    return "Could not set value: properties have different type.";
  }
  m_values = prop->m_values;
  m_size = prop->m_size;
  m_propSortedFlag = prop->m_propSortedFlag;
  // m_filter = prop->m_filter;
  // m_filterQuickRef = prop->m_filterQuickRef;
  // m_filterApplied = prop->m_filterApplied;
  return "";
}

//----------------------------------------------------------------------------------------------
/** Saves the time vector has time + start attribute */
template <typename TYPE> void TimeSeriesProperty<TYPE>::saveTimeVector(::NeXus::File *file) {
  std::vector<DateAndTime> times = this->timesAsVector();
  const DateAndTime &start = times.front();
  std::vector<double> timeSec(times.size());
  for (size_t i = 0; i < times.size(); i++)
    timeSec[i] = static_cast<double>(times[i].totalNanoseconds() - start.totalNanoseconds()) * 1e-9;
  file->writeData("time", timeSec);
  file->openData("time");
  file->putAttr("start", start.toISO8601String());
  file->closeData();
}

//----------------------------------------------------------------------------------------------
/** Helper function to save a TimeSeriesProperty<> */
template <> void TimeSeriesProperty<std::string>::saveProperty(::NeXus::File *file) {
  std::vector<std::string> values = this->valuesAsVector();
  if (values.empty())
    return;
  file->makeGroup(this->name(), "NXlog", true);

  // Find the max length of any string
  auto max_it = std::max_element(values.begin(), values.end(),
                                 [](const std::string &a, const std::string &b) { return a.size() < b.size(); });
  // Increment by 1 to have the 0 terminator
  size_t maxlen = max_it->size() + 1;
  // Copy into one array
  std::vector<char> strs(values.size() * maxlen);
  size_t index = 0;
  for (const auto &prop : values) {
    std::copy(prop.begin(), prop.end(), &strs[index]);
    index += maxlen;
  }

  std::vector<int> dims{static_cast<int>(values.size()), static_cast<int>(maxlen)};
  file->makeData("value", NXnumtype::CHAR, dims, true);
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

template <typename TYPE> void TimeSeriesProperty<TYPE>::saveProperty(::NeXus::File *file) {
  auto values = this->valuesAsVector();
  if (values.empty())
    return;
  file->makeGroup(this->name(), "NXlog", 1);
  file->writeData("value", values);
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
template <typename TYPE> Json::Value TimeSeriesProperty<TYPE>::valueAsJson() const { return Json::Value(value()); }

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
void TimeSeriesProperty<TYPE>::histogramData(const Types::Core::DateAndTime &tMin, const Types::Core::DateAndTime &tMax,
                                             std::vector<double> &counts) const {

  size_t nPoints = counts.size();
  if (nPoints == 0)
    return; // nothing to do

  auto t0 = static_cast<double>(tMin.totalNanoseconds());
  auto t1 = static_cast<double>(tMax.totalNanoseconds());
  if (t0 > t1)
    throw std::invalid_argument("invalid arguments for histogramData; tMax<tMin");

  double dt = (t1 - t0) / static_cast<double>(nPoints);

  for (auto &ev : m_values) {
    auto time = static_cast<double>(ev.time().totalNanoseconds());
    if (time < t0 || time >= t1)
      continue;
    auto ind = static_cast<size_t>((time - t0) / dt);
    counts[ind] += static_cast<double>(ev.value());
  }
}

template <>
void TimeSeriesProperty<std::string>::histogramData(const Types::Core::DateAndTime &tMin,
                                                    const Types::Core::DateAndTime &tMax,
                                                    std::vector<double> &counts) const {
  UNUSED_ARG(tMin);
  UNUSED_ARG(tMax);
  UNUSED_ARG(counts);
  throw std::runtime_error("histogramData is not implememnted for time series "
                           "properties containing strings");
}

/**
 * A view of the times as seen within a the TimeROI's regions.
 * A values will be excluded if its times lie outside the ROI's regions or coincides with the upper boundary
 * of a ROI region. For instance, time "2007-11-30T16:17:30" is excluded in the ROI
 * ["2007-11-30T16:17:00", "2007-11-30T16:17:30"). However, a value at time "2007-11-30T16:16:00" will be included
 * because it is valid at the beginning of the TimeROI.
 * @param roi :: ROI regions validating any query time.
 * @returns :: Vector of included values only.
 */
template <typename TYPE> std::vector<TYPE> TimeSeriesProperty<TYPE>::filteredValuesAsVector(const TimeROI *roi) const {
  if (roi && !roi->useAll()) {
    this->sortIfNecessary();
    std::vector<TYPE> filteredValues;
    if (roi->firstTime() > this->m_values.back().time()) {
      // Since the ROI starts after everything, just return the last value in the log
      filteredValues.emplace_back(this->m_values.back().value());
    } else { // only use the values in the filter - this is very similar to FilteredTimeSeriesProperty::applyFilter
      // the index into the m_values array of the time, or -1 (before) or m_values.size() (after)
      std::size_t index_current_log{0};
      for (const auto &splitter : roi->toTimeIntervals()) {
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
          filteredValues.emplace_back(this->m_values[index_current_log].value());
        }
        // go back one so the next splitter can add a value
        if (index_current_log > 0)
          index_current_log--;
      }
    }
    return filteredValues;
  } else {
    return this->valuesAsVector();
  }
}

template <typename TYPE> std::vector<TYPE> TimeSeriesProperty<TYPE>::filteredValuesAsVector() const {
  return this->valuesAsVector();
}

/**
 * Splitting interval for the whole time series.
 * The interval's starting time is that of the first log entry. The interval's ending time is that of the last log
 * entry plus an additional extra time. This extra time is the time span between the last two log entries.
 * The extra time ensures that the mean and the time-weighted mean are the same for time series
 * containing log entries equally spaced in time.
 * @returns :: Vector containing a single splitting interval.
 */
template <typename TYPE> std::vector<TimeInterval> TimeSeriesProperty<TYPE>::getTimeIntervals() const {
  std::vector<TimeInterval> intervals;
  auto lastInterval = this->nthInterval(this->size() - 1);
  intervals.emplace_back(firstTime(), lastInterval.stop());
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
