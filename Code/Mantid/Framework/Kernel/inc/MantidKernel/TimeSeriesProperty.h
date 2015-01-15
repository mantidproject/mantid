#ifndef MANTID_KERNEL_TIMESERIESPROPERTY_H_
#define MANTID_KERNEL_TIMESERIESPROPERTY_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/DateAndTime.h"
#include "MantidKernel/ITimeSeriesProperty.h"
#include "MantidKernel/Property.h"
#include "MantidKernel/Statistics.h"
#include <utility>

namespace Mantid {
namespace Kernel {

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
  /// Duration in seconds
  double duration;
};

//================================================================================================
/**
 * Class to hold unit value (DateAndTime, T)
 */
template <class TYPE> class TimeValueUnit {
private:
  Kernel::DateAndTime mtime;
  TYPE mvalue;

public:
  TimeValueUnit(const Kernel::DateAndTime &time, TYPE value) {
    mtime = time;
    mvalue = value;
  }

  ~TimeValueUnit() {}

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

  Kernel::DateAndTime time() const { return mtime; }

  void setTime(Kernel::DateAndTime newtime) { mtime = newtime; }

  TYPE value() const { return mvalue; }

  static bool valueCmp(const TimeValueUnit &lhs, const TimeValueUnit &rhs) {
    return (lhs.mvalue < rhs.mvalue);
  }
};
//========================================================================================================

/**
   A specialised Property class for holding a series of time-value pairs.

   Copyright &copy; 2007-2010 ISIS Rutherford Appleton Laboratory, NScD Oak
   Ridge National Laboratory & European Spallation Source

   This file is part of Mantid.

   Mantid is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   Mantid is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   File change history is stored at: <https://github.com/mantidproject/mantid>.
   Code Documentation is available at: <http://doxygen.mantidproject.org>
 */
template <typename TYPE>
class DLLExport TimeSeriesProperty : public Property,
                                     public ITimeSeriesProperty {
public:
  /// Constructor
  explicit TimeSeriesProperty(const std::string &name);
  /// Virtual destructor
  virtual ~TimeSeriesProperty();
  /// "Virtual" copy constructor
  TimeSeriesProperty<TYPE> *clone() const;

  /// Return the memory used by the property, in bytes
  size_t getMemorySize() const;
  /// Merge the given property with this one
  virtual TimeSeriesProperty<TYPE> &merge(Property *rhs);

  //--------------------------------------------------------------------------------------
  /// Add the value of another property
  virtual TimeSeriesProperty &operator+=(Property const *right);
  /// Deep comparison
  virtual bool operator==(const TimeSeriesProperty<TYPE> &right) const;
  /// Deep comparison (not equal).
  virtual bool operator!=(const TimeSeriesProperty<TYPE> &right) const;
  /// Deep comparison
  virtual bool operator==(const Property &right) const;
  /// Deep comparison (not equal).
  virtual bool operator!=(const Property &right) const;

  /// Set name of property
  void setName(const std::string name);

  /// Filter out a run by time.
  void filterByTime(const Kernel::DateAndTime &start,
                    const Kernel::DateAndTime &stop);
  /// Filter by a range of times
  void filterByTimes(const std::vector<SplittingInterval> &splittervec);

  /// Split out a time series property by time intervals.
  void splitByTime(std::vector<SplittingInterval> &splitter,
                   std::vector<Property *> outputs) const;
  /// Fill a TimeSplitterType that will filter the events by matching
  void makeFilterByValue(std::vector<SplittingInterval> &split, double min,
                         double max, double TimeTolerance = 0.0,
                         bool centre = false) const;
  /// Make sure an existing filter covers the full time range given
  void expandFilterToRange(std::vector<SplittingInterval> &split, double min,
                           double max, const TimeInterval &range) const;
  /// Calculate the time-weighted average of a property in a filtered range
  double
  averageValueInFilter(const std::vector<SplittingInterval> &filter) const;
  /// Calculate the time-weighted average of a property
  double timeAverageValue() const;

  ///  Return the time series as a correct C++ map<DateAndTime, TYPE>. All
  ///  values
  std::map<DateAndTime, TYPE> valueAsCorrectMap() const;
  ///  Return the time series's values as a vector<TYPE>
  std::vector<TYPE> valuesAsVector() const;
  ///  Return the time series as a correct C++ multimap<DateAndTime, TYPE>. All
  ///  values
  std::multimap<DateAndTime, TYPE> valueAsMultiMap() const;

  /// Return the time series's times as a vector<DateAndTime>
  std::vector<DateAndTime> timesAsVector() const;
  /// Return the series as list of times, where the time is the number of
  /// seconds since the start.
  std::vector<double> timesAsVectorSeconds() const;

  /// Add a value to the map using a DateAndTime object
  void addValue(const Kernel::DateAndTime &time, const TYPE value);
  /// Add a value to the map using a string time
  void addValue(const std::string &time, const TYPE value);
  /// Add a value to the map using a time_t
  void addValue(const std::time_t &time, const TYPE value);
  /// Adds vectors of values to the map. Should be much faster than repeated
  /// calls to addValue.
  void addValues(const std::vector<Kernel::DateAndTime> &times,
                 const std::vector<TYPE> &values);

  /// Returns the last time
  DateAndTime lastTime() const;
  /// Returns the first value regardless of filter
  TYPE firstValue() const;
  /// Returns the first time regardless of filter
  DateAndTime firstTime() const;
  /// Returns the last value
  TYPE lastValue() const;

  /// Returns the minimum value found in the series
  TYPE minValue() const;
  /// Returns the maximum value found in the series
  TYPE maxValue() const;

  /// Returns the number of values at UNIQUE time intervals in the time series
  int size() const;
  /// Returns the real size of the time series property map:
  int realSize() const;

  // ==== The following functions are specific to the odd mechanism of
  // FilterByLogValue =========
  /// Get the time series property as a string of 'time  value'
  std::string value() const;
  /// New method to return time series value pairs as std::vector<std::string>
  std::vector<std::string> time_tValue() const;
  /// Return the time series as a C++ map<DateAndTime, TYPE>
  std::map<DateAndTime, TYPE> valueAsMap() const;
  // ============================================================================================

  /// Set a property from a string
  std::string setValue(const std::string &);
  /// Set a property from a DataItem
  std::string setDataItem(const boost::shared_ptr<DataItem>);

  /// Deletes the series of values in the property
  void clear();
  /// Deletes all but the 'last entry' in the property
  void clearOutdated();
  /// Clears and creates a TimeSeriesProperty from these parameters
  void create(const Kernel::DateAndTime &start_time,
              const std::vector<double> &time_sec,
              const std::vector<TYPE> &new_values);
  /// Clears and creates a TimeSeriesProperty from these parameters
  void create(const std::vector<DateAndTime> &new_times,
              const std::vector<TYPE> &new_values);

  /// Returns the value at a particular time
  TYPE getSingleValue(const DateAndTime &t) const;
  /// Returns the value at a particular time
  TYPE getSingleValue(const DateAndTime &t, int &index) const;

  /// Returns n-th valid time interval, in a very inefficient way.
  TimeInterval nthInterval(int n) const;
  /// Returns n-th value of n-th interval in an incredibly inefficient way.
  TYPE nthValue(int n) const;
  /// Returns n-th time. NOTE: Complexity is order(n)! regardless of filter
  Kernel::DateAndTime nthTime(int n) const;

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
  std::string isValid() const;
  /// Returns the default value
  std::string getDefault() const;
  /// Returns if the value is at the default
  bool isDefault() const;

  /// Return a TimeSeriesPropertyStatistics object
  TimeSeriesPropertyStatistics getStatistics() const;

  /// Detects whether there are duplicated entries (of time) in property &
  /// eliminates them
  void eliminateDuplicates();

  /// Stringize the property
  std::string toString() const;

private:
  /// Sort the property into increasing times
  void sort() const;
  ///  Find the index of the entry of time t in the mP vector (sorted)
  int findIndex(Kernel::DateAndTime t) const;
  ///  Find the upper_bound of time t in container.
  int upperBound(Kernel::DateAndTime t, int istart, int iend) const;
  /// Apply a filter
  void applyFilter() const;
  /// A new algorithm to find Nth index.  It is simple and leave a lot work to
  /// the callers
  size_t findNthIndexFromQuickRef(int n) const;
  /// Set a value from another property
  virtual std::string setValueFromProperty(const Property &right);

  /// Holds the time series data
  mutable std::vector<TimeValueUnit<TYPE>> m_values;

  /// The number of values (or time intervals) in the time series. It can be
  /// different from m_propertySeries.size()
  mutable int m_size;

  /// Flag to state whether mP is sorted or not
  mutable TimeSeriesSortStatus m_propSortedFlag;

  /// The filter
  mutable std::vector<std::pair<Kernel::DateAndTime, bool>> m_filter;
  /// Quick reference regions for filter
  mutable std::vector<std::pair<size_t, size_t>> m_filterQuickRef;
  /// True if a filter has been applied
  mutable bool m_filterApplied;
};

/// Function filtering double TimeSeriesProperties according to the requested
/// statistics.
double DLLExport
    filterByStatistic(TimeSeriesProperty<double> const *const propertyToFilter,
                      Kernel::Math::StatisticType statistic_type);

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_TIMESERIESPROPERTY_H_*/
