// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"

#include <vector>

namespace Mantid {
namespace Types {
namespace Core {
class DateAndTime;
}
} // namespace Types
namespace Kernel {
//---------------------------------------------------------------------------
// Forward Declarations
//---------------------------------------------------------------------------
namespace Math {
enum StatisticType : int;
}
class SplittingInterval;
class TimeInterval;
struct TimeSeriesPropertyStatistics;
class Property;
class TimeROI;
/** A non-templated interface to a TimeSeriesProperty.
 */
class MANTID_KERNEL_DLL ITimeSeriesProperty {
public:
  /// Fill a SplittingIntervalVec that will filter the events by matching
  virtual TimeROI makeFilterByValue(double min, double max, bool expand = false,
                                    const TimeInterval &expandRange = TimeInterval(0, 1), double TimeTolerance = 0.,
                                    bool centre = true, const TimeROI *existingROI = nullptr) const = 0;
  /// Fill a SplittingIntervalVec that will filter the events by matching
  virtual void makeFilterByValue(std::vector<SplittingInterval> &split, double min, double max, double TimeTolerance,
                                 bool centre = true) const = 0;
  /// Make sure an existing filter covers the full time range given
  virtual void expandFilterToRange(std::vector<SplittingInterval> &split, double min, double max,
                                   const TimeInterval &range) const = 0;
  // Provide a new instance of the ITimeSeriesProperty with shifted time values
  // After trying to use return type covariance, but that showed Error C2908
  // Using property seemed to be the most straightforward solution.
  virtual Property *cloneWithTimeShift(const double timeShift) const = 0;

  // Create a partial copy of this ITimeSeriesProperty-derived object according to a TimeROI. The partially cloned
  // object will include all time values enclosed by the ROI regions defined as [roi_begin,roi_end], plus the values
  // immediately before and after an ROI region, if available.
  virtual Property *cloneInTimeROI(const TimeROI &timeROI) const = 0;

  /// Return the time series's times as a vector<DateAndTime>
  virtual std::vector<Types::Core::DateAndTime> timesAsVector() const = 0;
  /** Returns the calculated time weighted average value.
   * @param timeRoi  Object that holds information about when the time measurement was active.
   * @return The time-weighted average value of the log when the time measurement was active.
   */
  virtual double timeAverageValue(const TimeROI *timeRoi = nullptr) const = 0;
  /** Returns the calculated time weighted mean and standard deviation values.
   * @param timeRoi  Object that holds information about when the time measurement was active.
   */
  virtual std::pair<double, double> timeAverageValueAndStdDev(const Kernel::TimeROI *timeRoi = nullptr) const = 0;
  /// Return a TimeSeriesPropertyStatistics object
  virtual TimeSeriesPropertyStatistics getStatistics(const TimeROI *roi = nullptr) const = 0;
  /// Filtering the series according to the selected statistical measure
  virtual double extractStatistic(Math::StatisticType selection, const TimeROI * = nullptr) const = 0;
  /// Returns the real size of the time series property map:
  virtual int realSize() const = 0;
  /// Deletes the series of values in the property
  virtual void clear() = 0;
  /// Deletes all but the 'last entry' in the property
  virtual void clearOutdated() = 0;

  // Returns whether the time series has been filtered
  virtual bool isFiltered() const = 0;

  // Remove time values outside the TimeROI regions defined as [roi_begin,roi_end].
  // However, keep the values immediately before and after each ROI region, if available.
  virtual void removeDataOutsideTimeROI(const TimeROI &timeRoi) = 0;

  /// Virtual destructor
  virtual ~ITimeSeriesProperty() = default;
};

} // namespace Kernel
} // namespace Mantid
