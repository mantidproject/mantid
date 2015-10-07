#ifndef MANTID_KERNEL_FILTEREDTIMESERIESPROPERTY_H_
#define MANTID_KERNEL_FILTEREDTIMESERIESPROPERTY_H_
/**
  Copyright &copy; 2012 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

  File change history is stored at: <https://github.com/mantidproject/mantid>
  Code Documentation is available at: <http://doxygen.mantidproject.org>
*/
#include "MantidKernel/ClassMacros.h"
#include "MantidKernel/DllConfig.h"
#include "MantidKernel/TimeSeriesProperty.h"

namespace Mantid {
namespace Kernel {

/**
 * Templated class that defines a filtered time series but
 * still gives access to the original data.
 */
template <typename HeldType>
class DLLExport FilteredTimeSeriesProperty
    : public TimeSeriesProperty<HeldType> {

public:
  /// Construct with a source time series & a filter property
  FilteredTimeSeriesProperty(TimeSeriesProperty<HeldType> *seriesProp,
                             const TimeSeriesProperty<bool> &filterProp,
                             const bool transferOwnserhip = false);
  /// Destructor
  ~FilteredTimeSeriesProperty();

  /// Access the unfiltered log
  const TimeSeriesProperty<HeldType> *unfiltered() const;

private:
  DISABLE_DEFAULT_CONSTRUCT(FilteredTimeSeriesProperty)

  /// The original unfiltered property as an owned pointer
  const TimeSeriesProperty<HeldType> *m_unfiltered;
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_FILTEREDTIMESERIESPROPERTY_H_ */
