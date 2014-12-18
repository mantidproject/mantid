#include "MantidKernel/FilteredTimeSeriesProperty.h"

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
    const TimeSeriesProperty<bool> &filterProp, const bool transferOwnership)
    : TimeSeriesProperty<HeldType>(*seriesProp), m_unfiltered(NULL) {
  if (transferOwnership)
    m_unfiltered = seriesProp;
  else
    m_unfiltered = seriesProp->clone();

  // Now filter us with the filter
  this->filterWith(&filterProp);
}

/**
 * Destructor
 */
template <typename HeldType>
FilteredTimeSeriesProperty<HeldType>::~FilteredTimeSeriesProperty() {
  delete m_unfiltered;
}

/**
 * Access the unfiltered log
 * @returns A pointer to the unfiltered property
 */
template <typename HeldType>
const TimeSeriesProperty<HeldType> *
FilteredTimeSeriesProperty<HeldType>::unfiltered() const {
  return m_unfiltered;
}

/// @cond
// -------------------------- Macro to instantiation concrete types
// --------------------------------
#define INSTANTIATE(TYPE)                                                      \
  template MANTID_KERNEL_DLL class FilteredTimeSeriesProperty<TYPE>;

// -------------------------- Concrete instantiation
// -----------------------------------------------
INSTANTIATE(int);
INSTANTIATE(long);
INSTANTIATE(long long);
INSTANTIATE(unsigned int);
INSTANTIATE(unsigned long);
INSTANTIATE(unsigned long long);
INSTANTIATE(float);
INSTANTIATE(double);
INSTANTIATE(std::string);
INSTANTIATE(bool);

///@endcond

} // namespace Kernel
} // namespace Mantid
