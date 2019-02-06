// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_KERNEL_TIMESERIESPROPERTY_H_
#define MANTID_KERNEL_TIMESERIESPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/ITimeSeriesProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Statistics.h"
#include <cstdint>
#include <utility>

// Forward declare
namespace NeXus {
class File;
}

namespace Mantid {
namespace Kernel {
class DataItem;
class SplittingInterval;

enum TimeSeriesSortStatus { TSUNKNOWN, TSUNSORTED, TSSORTED };

//=========================================================================
/** Struct holding some useful statistics for a TimeSeriesProperty
 *
 */
struct TimeSeriesPropertyStatistics {
  /// Minimum value
  double minimum;
  /// Maximum value
  double maximum;
  /// Mean value
  double mean;
  /// Median value
  double median;
  /// standard_deviation of the values
  double standard_deviation;
  /// time weighted average
  double time_mean;
  /// time weighted standard deviation
  double time_standard_deviation;
  /// Duration in seconds
  double duration;
};

//================================================================================================
/**
 * Class to hold unit value (DateAndTime, T)
 */
template <class TYPE> class TimeValueUnit {
private:
  Types::Core::DateAndTime mtime;
  TYPE mvalue;

public:
  TimeValueUnit(const Types::Core::DateAndTime &time, TYPE value) {
    mtime = time;
    mvalue = value;
  }

  ~TimeValueUnit() = default;

  bool operator>(const TimeValueUnit &rhs) { return (mtime > rhs.mtime); }

  friend bool operator>(const TimeValueUnit &lhs, const TimeValueUnit &rhs) {
    return (lhs.mtime > rhs.mtime);
  }

  bool operator==(const TimeValueUnit &rhs) { return (mtime == rhs.mtime); }

  friend bool operator==(const TimeValueUnit &lhs, const TimeValueUnit &rhs) {
    return (lhs.mtime == rhs.mtime);
  }

  bool operator<(const TimeValueUnit &rhs) { return (mtime < rhs.mtime); }

  friend bool operator<(const TimeValueUnit &lhs, const TimeValueUnit &rhs) {
    return (lhs.mtime < rhs.mtime);
  }

  Types::Core::DateAndTime time() const { return mtime; }

  void setTime(Types::Core::DateAndTime newtime) { mtime = newtime; }

  TYPE value() const { return mvalue; }

  static bool valueCmp(const TimeValueUnit &lhs, const TimeValueUnit &rhs) {
    return (lhs.mvalue < rhs.mvalue);
  }
};
//========================================================================================================

/**
   A specialised Property class for holding a series of time-value pairs.
 */
template <typename TYPE>
class DLLExport TimeSeriesProperty : public Property,
                                     public ITimeSeriesProperty {
public:
  /// Constructor
  explicit TimeSeriesProperty(const std::string &name);

  /// Virtual destructor
  ~TimeSeriesProperty() override;
  /// "Virtual" copy constructor
  TimeSeriesProperty<TYPE> *clone() const override;
  //
  /// Return time series property, containing time derivative of current
  /// property
  std::unique_ptr<TimeSeriesProperty<double>> getDerivative() const;

  void saveProperty(::NeXus::File *file) override;
  Json::Value valueAsJson() const override;

  /// "Virtual" copy constructor with a time shift in seconds
  Property *cloneWithTimeShift(const double timeShift) const override;
  /// Return the memory used by the property, in bytes
  size_t getMemorySize() const override;
  /// Merge the given property with this one
  TimeSeriesProperty<TYPE> &merge(Property *rhs) override;

  //--------------------------------------------------------------------------------------
  /// Add the value of another property
  TimeSeriesProperty &operator+=(Property const *right) override;
  /// Deep comparison
  virtual bool operator==(const TimeSeriesProperty<TYPE> &right) const;
  /// Deep comparison (not equal).
  virtual bool operator!=(const TimeSeriesProperty<TYPE> &right) const;
  /// Deep comparison
  virtual bool operator==(const Property &right) const;
  /// Deep comparison (not equal).
  virtual bool operator!=(const Property &right) const;

  /// Set name of property
  void setName(const std::string &name);

  /// Filter out a run by time.
  void filterByTime(const Types::Core::DateAndTime &start,
                    const Types::Core::DateAndTime &stop) override;
  /// Filter by a range of times
  void filterByTimes(const std::vector<SplittingInterval> &splittervec);

  /// Split out a time series property by time intervals.
  void splitByTime(std::vector<SplittingInterval> &splitter,
                   std::vector<Property *> outputs,
                   bool isPeriodic) const override;

  /// New split method
  void
  splitByTimeVector(std::vector<Types::Core::DateAndTime> &splitter_time_vec,
                    std::vector<int> &target_vec,
                    std::vector<TimeSeriesProperty *> outputs);

  /// Fill a TimeSplitterType that will filter the events by matching
  void makeFilterByValue(std::vector<SplittingInterval> &split, double min,
                         double max, double TimeTolerance = 0.0,
                         bool centre = false) const override;
  /// Make sure an existing filter covers the full time range given
  void expandFilterToRange(std::vector<SplittingInterval> &split, double min,
                           double max,
                           const TimeInterval &range) const override;
  /// Calculate the time-weighted average of a property in a filtered range
  double averageValueInFilter(
      const std::vector<SplittingInterval> &filter) const override;
  /// @copydoc Mantid::Kernel::ITimeSeriesProperty::averageAndStdDevInFilter()
  std::pair<double, double> averageAndStdDevInFilter(
      const std::vector<SplittingInterval> &filter) const override;
  /// @copydoc Mantid::Kernel::ITimeSeriesProperty::timeAverageValue()
  double timeAverageValue() const override;
  /// generate constant time-step histogram from the property values
  void histogramData(const Types::Core::DateAndTime &tMin,
                     const Types::Core::DateAndTime &tMax,
                     std::vector<double> &counts) const;

  ///  Return the time series as a correct C++ map<DateAndTime, TYPE>. All
  ///  values
  std::map<Types::Core::DateAndTime, TYPE> valueAsCorrectMap() const;
  ///  Return the time series's values (unfiltered) as a vector<TYPE>
  std::vector<TYPE> valuesAsVector() const;
  ///  Return the time series as a correct C++ multimap<DateAndTime, TYPE>. All
  ///  values
  std::multimap<Types::Core::DateAndTime, TYPE> valueAsMultiMap() const;
  /// Get filtered values as a vector
  std::vector<TYPE> filteredValuesAsVector() const;

  /// Return the time series's times as a vector<DateAndTime>
  std::vector<Types::Core::DateAndTime> timesAsVector() const override;
  /// Return the series as list of times, where the time is the number of
  /// seconds since the start.
  std::vector<double> timesAsVectorSeconds() const;

  /// Add a value to the map using a DateAndTime object
  void addValue(const Types::Core::DateAndTime &time, const TYPE value);
  /// Add a value to the map using a string time
  void addValue(const std::string &time, const TYPE value);
  /// Add a value to the map using a time_t
  void addValue(const std::time_t &time, const TYPE value);
  /// Adds vectors of values to the map. Should be much faster than repeated
  /// calls to addValue.
  void addValues(const std::vector<Types::Core::DateAndTime> &times,
                 const std::vector<TYPE> &values);
  /// Replaces the time series with new values time series values
  void replaceValues(const std::vector<Types::Core::DateAndTime> &times,
                     const std::vector<TYPE> &values);

  /// Returns the last time
  Types::Core::DateAndTime lastTime() const;
  /// Returns the first value regardless of filter
  TYPE firstValue() const;
  /// Returns the first time regardless of filter
  Types::Core::DateAndTime firstTime() const;
  /// Returns the last value
  TYPE lastValue() const;

  /// Returns the minimum value found in the series
  TYPE minValue() const;
  /// Returns the maximum value found in the series
  TYPE maxValue() const;

  /// Returns the number of values at UNIQUE time intervals in the time series
  int size() const override;
  /// Returns the real size of the time series property map:
  int realSize() const override;

  // ==== The following functions are specific to the odd mechanism of
  // FilterByLogValue =========
  /// Get the time series property as a string of 'time  value'
  std::string value() const override;
  /// New method to return time series value pairs as std::vector<std::string>
  std::vector<std::string> time_tValue() const;
  /// Return the time series as a C++ map<DateAndTime, TYPE>
  std::map<Types::Core::DateAndTime, TYPE> valueAsMap() const;
  // ============================================================================================

  /// Set a property from a string
  std::string setValue(const std::string &) override;
  /// Set a property from a string
  std::string setValueFromJson(const Json::Value &) override;
  /// Set a property from a DataItem
  std::string setDataItem(const boost::shared_ptr<DataItem>) override;

  /// Deletes the series of values in the property
  void clear() override;
  /// Deletes all but the 'last entry' in the property
  void clearOutdated() override;
  /// Clears and creates a TimeSeriesProperty from these parameters
  void create(const Types::Core::DateAndTime &start_time,
              const std::vector<double> &time_sec,
              const std::vector<TYPE> &new_values);
  /// Clears and creates a TimeSeriesProperty from these parameters
  void create(const std::vector<Types::Core::DateAndTime> &new_times,
              const std::vector<TYPE> &new_values);

  /// Returns the value at a particular time
  TYPE getSingleValue(const Types::Core::DateAndTime &t) const;
  /// Returns the value at a particular time
  TYPE getSingleValue(const Types::Core::DateAndTime &t, int &index) const;

  /// Returns n-th valid time interval, in a very inefficient way.
  TimeInterval nthInterval(int n) const;
  /// Returns n-th value of n-th interval in an incredibly inefficient way.
  TYPE nthValue(int n) const;
  /// Returns n-th time. NOTE: Complexity is order(n)! regardless of filter
  Types::Core::DateAndTime nthTime(int n) const;

  /// Divide the property into  allowed and disallowed time intervals according
  /// to \a filter.
  void filterWith(const TimeSeriesProperty<bool> *filter);
  /// Restores the property to the unsorted state
  void clearFilter();

  /// Updates size()
  void countSize() const;

  /// Check if str has the right time format
  static bool isTimeString(const std::string &str);

  /// This doesn't check anything -we assume these are always valid
  std::string isValid() const override;
  /// Returns the default value
  std::string getDefault() const override;
  /// Returns if the value is at the default
  bool isDefault() const override;

  /// Return a TimeSeriesPropertyStatistics object
  TimeSeriesPropertyStatistics getStatistics() const;

  /// Detects whether there are duplicated entries (of time) in property &
  /// eliminates them
  void eliminateDuplicates();

  /// Stringize the property
  std::string toString() const;

  /**Reserve memory for efficient adding values to existing property
   * makes sense only when you have reasonably precise estimate of the
   * total size you'll need easily available in advance.  */
  void reserve(size_t size) { m_values.reserve(size); };

  /// If filtering by log, get the time intervals for splitting
  std::vector<Mantid::Kernel::SplittingInterval> getSplittingIntervals() const;

private:
  //----------------------------------------------------------------------------------------------
  /// Saves the time vector has time + start attribute
  void saveTimeVector(::NeXus::File *file);
  /// Sort the property into increasing times, if not already sorted
  void sortIfNecessary() const;
  ///  Find the index of the entry of time t in the mP vector (sorted)
  int findIndex(Types::Core::DateAndTime t) const;
  ///  Find the upper_bound of time t in container.
  int upperBound(Types::Core::DateAndTime t, int istart, int iend) const;
  /// Apply a filter
  void applyFilter() const;
  /// A new algorithm to find Nth index.  It is simple and leave a lot work to
  /// the callers
  size_t findNthIndexFromQuickRef(int n) const;
  /// Set a value from another property
  std::string setValueFromProperty(const Property &right) override;
  /// Find if time lies in a filtered region
  bool isTimeFiltered(const Types::Core::DateAndTime &time) const;
  /// Time weighted mean and standard deviation
  std::pair<double, double> timeAverageValueAndStdDev() const;

  /// Holds the time series data
  mutable std::vector<TimeValueUnit<TYPE>> m_values;

  /// The number of values (or time intervals) in the time series. It can be
  /// different from m_propertySeries.size()
  mutable int m_size;

  /// Flag to state whether mP is sorted or not
  mutable TimeSeriesSortStatus m_propSortedFlag;

  /// The filter
  mutable std::vector<std::pair<Types::Core::DateAndTime, bool>> m_filter;
  /// Quick reference regions for filter
  mutable std::vector<std::pair<size_t, size_t>> m_filterQuickRef;
  /// True if a filter has been applied
  mutable bool m_filterApplied;
};

/// Function filtering double TimeSeriesProperties according to the requested
/// statistics.
double DLLExport
filterByStatistic(TimeSeriesProperty<double> const *const propertyToFilter,
                  Kernel::Math::StatisticType statisticType);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_TIMESERIESPROPERTY_H_*/
