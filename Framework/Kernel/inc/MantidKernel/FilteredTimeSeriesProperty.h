// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
using namespace Types::Core;
namespace Kernel {

class SplittingInterval;

/**
 * Templated class that defines a filtered time series but
 * still gives access to the original data.
 */
template <typename HeldType> class MANTID_KERNEL_DLL FilteredTimeSeriesProperty : public TimeSeriesProperty<HeldType> {

public:
  explicit FilteredTimeSeriesProperty(const std::string &name);
  /// Construct with a source time series & a filter property
  FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp, const TimeSeriesProperty<bool> &filterProp);
  FilteredTimeSeriesProperty(const std::string &name, const std::vector<Types::Core::DateAndTime> &times,
                             const std::vector<HeldType> &values);

  FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp);

  /// Construct with a source time series to take ownership of & a filter
  /// property
  FilteredTimeSeriesProperty(std::unique_ptr<const TimeSeriesProperty<HeldType>> seriesProp,
                             const TimeSeriesProperty<bool> &filterProp);
  /// "Virtual" copy constructor
  FilteredTimeSeriesProperty<HeldType> *clone() const override;

  FilteredTimeSeriesProperty(const FilteredTimeSeriesProperty &prop);

  /// Destructor
  ~FilteredTimeSeriesProperty() override;

  /// Disable default constructor
  FilteredTimeSeriesProperty() = delete;

  /// Get filtered values as a vector
  std::vector<HeldType> filteredValuesAsVector(const Kernel::TimeROI *roi) const override;
  // overload method rather than default value so python bindings work
  std::vector<HeldType> filteredValuesAsVector() const override;
  /// Get filtered times as a vector
  std::vector<Types::Core::DateAndTime> filteredTimesAsVector(const Kernel::TimeROI *roi) const override;
  // overload method rather than default value so python bindings work
  std::vector<Types::Core::DateAndTime> filteredTimesAsVector() const override;

  /// Returns n-th valid time interval, in a very inefficient way.
  TimeInterval nthInterval(int n) const override;
  /// Returns n-th value of n-th interval in an incredibly inefficient way.
  HeldType nthValue(int n) const override;
  Types::Core::DateAndTime nthTime(int n) const override;

  /// Divide the property into  allowed and disallowed time intervals according
  /// to a filter.
  void filterWith(const TimeSeriesProperty<bool> *filter);
  /// Divide the property into  allowed and disallowed time intervals according
  /// to a filter.
  void filterWith(const TimeROI &filter);
  /// Restores the property to the unsorted state
  void clearFilter() const;
  // Returns whether the time series has been filtered
  bool isFiltered() const override { return m_filterApplied; }

  int size() const override;

  /// Access the unfiltered log
  const TimeSeriesProperty<HeldType> *unfiltered() const;

  /// If filtering by log, get the time intervals for splitting
  std::vector<Mantid::Kernel::TimeInterval> getTimeIntervals() const override;

  const Kernel::TimeROI &getTimeROI() const;
  double timeAverageValue(const TimeROI *timeRoi = nullptr) const override;

  bool operator==(const TimeSeriesProperty<HeldType> &right) const override;
  bool operator==(const Property &right) const override;

private:
  /// Apply a filter
  void applyFilter() const;
  /// Updates size()
  void countSize() const;
  /// Clear out the applied filter
  void clearFilterCache() const;

  /// Set a value from another property
  std::string setValueFromProperty(const Property &right) override;

  TimeROI *intersectFilterWithOther(const TimeROI *other) const;

  /// The filter
  mutable std::unique_ptr<TimeROI> m_filter;
  /// Maps the index supplied to nthValue and nthInterval to values in m_value.
  mutable std::vector<size_t> m_filterMap;
  /// Cached values for the time intervals inside the filter
  mutable std::vector<TimeInterval> m_filterIntervals;
  /// True if a filter has been applied
  mutable bool m_filterApplied;
};

} // namespace Kernel
} // namespace Mantid
