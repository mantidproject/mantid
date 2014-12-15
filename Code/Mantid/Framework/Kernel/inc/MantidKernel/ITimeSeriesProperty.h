#ifndef MANTID_KERNEL_ITIMESERIESPROPERTY_H_
#define MANTID_KERNEL_ITIMESERIESPROPERTY_H_

#include <vector>

namespace Mantid
{
namespace Kernel
{
  //---------------------------------------------------------------------------
  // Forward Declarations
  //---------------------------------------------------------------------------
  class DateAndTime;
  class SplittingInterval;
  class TimeInterval;

/** A non-templated interface to a TimeSeriesProperty.

    Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge National Laboratory & European Spallation Source

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
*/
class ITimeSeriesProperty
{
public:
  /// Fill a TimeSplitterType that will filter the events by matching
  virtual void makeFilterByValue(std::vector<SplittingInterval>& split, double min, double max, double TimeTolerance, bool centre=true) const = 0;
  /// Make sure an existing filter covers the full time range given
  virtual void expandFilterToRange(std::vector<SplittingInterval>& split, double min, double max, const TimeInterval & range) const = 0;
  /// Calculate the time-weighted average of a property in a filtered range
  virtual double averageValueInFilter(const std::vector<SplittingInterval>& filter) const = 0;
  /// Return the time series's times as a vector<DateAndTime>
  virtual std::vector<DateAndTime> timesAsVector() const = 0;
  /// Returns the real size of the time series property map:
  virtual int realSize() const = 0;
  /// Deletes the series of values in the property
  virtual void clear() = 0;
  /// Deletes all but the 'last entry' in the property
  virtual void clearOutdated() = 0;

  /// Virtual destructor
  virtual ~ITimeSeriesProperty() {}
};

} // namespace Kernel
} // namespace Mantid

#endif /*MANTID_KERNEL_ITIMESERIESPROPERTY_H_*/
