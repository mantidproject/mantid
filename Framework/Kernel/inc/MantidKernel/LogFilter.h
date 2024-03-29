// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2007 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

//----------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------
#include "MantidKernel/DllConfig.h"

#include <memory>
#include <vector>

namespace Mantid {
namespace Kernel {

class TimeROI;

//----------------------------------------------------------------------
// Forward declarations
//----------------------------------------------------------------------
class Property;
template <class TYPE> class FilteredTimeSeriesProperty;
template <class TYPE> class TimeSeriesProperty;

/**
This class is for filtering TimeSeriesProperty data
 */
class MANTID_KERNEL_DLL LogFilter {
public:
  /// Disable default constructor
  LogFilter() = delete;

  /// Disable copy and assignment operator
  LogFilter(const LogFilter &) = delete;
  LogFilter &operator=(const LogFilter &) = delete;

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
  inline FilteredTimeSeriesProperty<double> *data() const { return m_prop.get(); }
  /// Returns a reference to the filter
  inline const TimeSeriesProperty<bool> *filter() const { return m_filter.get(); }
  /// Clears filters
  void clear();

private:
  /// Converts the given property to a TimeSeriesProperty<double>, throws if
  /// invalid.
  FilteredTimeSeriesProperty<double> *convertToTimeSeriesOfDouble(const Property *prop);
  void setFilter(const TimeROI &roi, const bool filterOpenEnded);

  /// Owned pointer to the filtered property
  std::unique_ptr<FilteredTimeSeriesProperty<double>> m_prop;
  /// Owned pointer to the filter mask
  std::unique_ptr<TimeSeriesProperty<bool>> m_filter;
};

} // namespace Kernel
} // namespace Mantid
