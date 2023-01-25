// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/System.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
using namespace Types::Core;
namespace Kernel {

class SplittingInterval;

/**
 * Templated class that defines a filtered time series but
 * still gives access to the original data.
 */
template <typename HeldType> class DLLExport FilteredTimeSeriesProperty : public TimeSeriesProperty<HeldType> {

public:
  explicit FilteredTimeSeriesProperty(const std::string &name);
  /// Construct with a source time series & a filter property
  FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp, const TimeSeriesProperty<bool> &filterProp);

  /// Construct with a source time series to take ownership of & a filter
  /// property
  FilteredTimeSeriesProperty(std::unique_ptr<const TimeSeriesProperty<HeldType>> seriesProp,
                             const TimeSeriesProperty<bool> &filterProp);
  /// "Virtual" copy constructor
  FilteredTimeSeriesProperty<HeldType> *clone() const override;

  FilteredTimeSeriesProperty(const FilteredTimeSeriesProperty &prop)
      : TimeSeriesProperty<HeldType>(prop.name(), prop.timesAsVector(), prop.valuesAsVector()),
        m_unfiltered(prop.unfiltered()->clone()), m_filter(prop.m_filter), m_filterQuickRef(prop.m_filterQuickRef),
        m_filterApplied(prop.m_filterApplied) {}

  /// Destructor
  ~FilteredTimeSeriesProperty() override;

  /// Disable default constructor
  FilteredTimeSeriesProperty() = delete;

  /// Get filtered values as a vector
  std::vector<HeldType> filteredValuesAsVector() const;
  /// Get filtered times as a vector
  std::vector<Types::Core::DateAndTime> filteredTimesAsVector() const;

  /// Returns n-th valid time interval, in a very inefficient way.
  TimeInterval nthInterval(int n) const;
  /// Returns n-th value of n-th interval in an incredibly inefficient way.
  HeldType nthValue(int n) const;

  /// Divide the property into  allowed and disallowed time intervals according
  /// to \a filter.
  void filterWith(const TimeSeriesProperty<bool> *filter);
  /// Restores the property to the unsorted state
  void clearFilter();
  // Returns whether the time series has been filtered
  bool isFiltered() const override { return m_filterApplied; }

  /// Updates size()
  void countSize() const;

  /// Access the unfiltered log
  const TimeSeriesProperty<HeldType> *unfiltered() const;

  /// If filtering by log, get the time intervals for splitting
  std::vector<Mantid::Kernel::SplittingInterval> getSplittingIntervals() const;

private:
  /// Apply a filter
  void applyFilter() const;
  /// A new algorithm to find Nth index.  It is simple and leave a lot work to
  /// the callers
  size_t findNthIndexFromQuickRef(int n) const;

  /// Set a value from another property
  std::string setValueFromProperty(const Property &right) override;

  /// Find if time lies in a filtered region
  bool isTimeFiltered(const Types::Core::DateAndTime &time) const;

  /// The original unfiltered property as an owned pointer
  std::unique_ptr<const TimeSeriesProperty<HeldType>> m_unfiltered;

  /// The filter
  mutable std::vector<std::pair<Types::Core::DateAndTime, bool>> m_filter;
  /// Quick reference regions for filter
  mutable std::vector<std::pair<size_t, size_t>> m_filterQuickRef;
  /// True if a filter has been applied
  mutable bool m_filterApplied;
};

} // namespace Kernel
} // namespace Mantid
