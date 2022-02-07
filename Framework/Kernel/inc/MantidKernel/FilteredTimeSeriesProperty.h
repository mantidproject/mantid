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
namespace Kernel {

/**
 * Templated class that defines a filtered time series but
 * still gives access to the original data.
 */
template <typename HeldType> class DLLExport FilteredTimeSeriesProperty : public TimeSeriesProperty<HeldType> {

public:
  /// Construct with a source time series & a filter property
  FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp, const TimeSeriesProperty<bool> &filterProp);

  /// Construct with a source time series to take ownership of & a filter
  /// property
  FilteredTimeSeriesProperty(std::unique_ptr<const TimeSeriesProperty<HeldType>> seriesProp,
                             const TimeSeriesProperty<bool> &filterProp);
  /// Destructor
  ~FilteredTimeSeriesProperty() override;

  /// Disable default constructor
  FilteredTimeSeriesProperty() = delete;

  /// Access the unfiltered log
  const TimeSeriesProperty<HeldType> *unfiltered() const;

private:
  /// The original unfiltered property as an owned pointer
  std::unique_ptr<const TimeSeriesProperty<HeldType>> m_unfiltered;
};

} // namespace Kernel
} // namespace Mantid
