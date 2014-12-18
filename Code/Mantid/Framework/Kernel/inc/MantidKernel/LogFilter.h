#ifndef MANTID_KERNEL_LOGFILTER_H_
#define MANTID_KERNEL_LOGFILTER_H_

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Exception.h"

#ifndef Q_MOC_RUN
#include <boost/scoped_ptr.hpp>
#endif

namespace Mantid {
namespace Kernel {

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class Property;
template <class TYPE> class TimeSeriesProperty;

/**
This class is for filtering TimeSeriesProperty data

Copyright &copy; 2007-12 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
National Laboratory & European Spallation Source

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
class MANTID_KERNEL_DLL LogFilter {
public:
  /// Constructor taking a filter. Allows filters to be combined without being
  /// applied to a property
  LogFilter(const TimeSeriesProperty<bool> &filter);
  /// Constructor taking any property type
  LogFilter(const Property *prop);
  /// Constructor from a TimeSeriesProperty<double> object to avoid overhead of
  /// casts
  LogFilter(const TimeSeriesProperty<double> *timeSeries);

  /// Adds a filter using boolean AND
  void addFilter(const TimeSeriesProperty<bool> &filter);
  ///  Returns reference to the filtered property
  inline TimeSeriesProperty<double> *data() const { return m_prop.get(); }
  /// Returns a reference to the filter
  inline const TimeSeriesProperty<bool> *filter() const {
    return m_filter.get();
  }
  /// Clears filters
  void clear();

private:
  DISABLE_DEFAULT_CONSTRUCT(LogFilter);
  DISABLE_COPY_AND_ASSIGN(LogFilter);

  /// Converts the given property to a TimeSeriesProperty<double>, throws if
  /// invalid.
  TimeSeriesProperty<double> *convertToTimeSeriesOfDouble(const Property *prop);

  /// Owned pointer to the filtered property
  boost::scoped_ptr<TimeSeriesProperty<double>> m_prop;
  /// Owned pointer to the filter mask
  boost::scoped_ptr<TimeSeriesProperty<bool>> m_filter;
};

} // namespace Kernel
} // namespace Mantid

#endif // LOG_FILTER_H
