// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidKernel/FilteredTimeSeriesProperty.h"
#include "MantidKernel/DllConfig.h"
#include <string>

using namespace Mantid::Kernel;

namespace Mantid {
namespace Kernel {

/**
 * Construct with a source time series & a filter property
 * @param seriesProp :: A pointer to a property to filter.
 * @param filterProp :: A boolean series property to filter on
 * @param transferOwnership :: Flag marking whether this object takes
 * ownership of the time series (default = false). Avoids unnecessary
 * clones when the original is going to be deleted anyway
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(
    TimeSeriesProperty<HeldType> *seriesProp,
    const TimeSeriesProperty<bool> &filterProp)
    : TimeSeriesProperty<HeldType>(*seriesProp), m_unfiltered(nullptr) {

  m_unfiltered =
      std::unique_ptr<const TimeSeriesProperty<HeldType>>(seriesProp->clone());

  // Now filter us with the filter
  this->filterWith(&filterProp);
}

/**
 * Construct with a source time series & a filter property
 * @param seriesProp :: A mart pointer to take ownership of pointer to a
 * property to filter.
 * @param filterProp :: A boolean series property to filter on
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::FilteredTimeSeriesProperty(
    std::unique_ptr<const TimeSeriesProperty<HeldType>> seriesProp,
    const TimeSeriesProperty<bool> &filterProp)
    : TimeSeriesProperty<HeldType>(*seriesProp) {
  m_unfiltered = std::move(seriesProp);
  // Now filter us with the filter
  this->filterWith(&filterProp);
}

/**
 * Destructor
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::~FilteredTimeSeriesProperty() {}

/**
 * Access the unfiltered log
 * @returns A pointer to the unfiltered property
 */
template <typename HeldType>
const TimeSeriesProperty<HeldType> *
FilteredTimeSeriesProperty<HeldType>::unfiltered() const {
  return m_unfiltered.get();
}

/// @cond
// -------------------------- Macro to instantiation concrete types
// --------------------------------
#define INSTANTIATE(TYPE)                                                      \
  template class MANTID_KERNEL_DLL FilteredTimeSeriesProperty<TYPE>;

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

///@endcond

} // namespace Kernel
} // namespace Mantid
