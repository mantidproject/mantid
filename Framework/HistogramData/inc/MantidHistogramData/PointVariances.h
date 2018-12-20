#ifndef MANTID_HISTOGRAMDATA_POINTVARIANCES_H_
#define MANTID_HISTOGRAMDATA_POINTVARIANCES_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramDx.h"
#include "MantidHistogramData/VarianceVectorOf.h"

namespace Mantid {
namespace HistogramData {

class PointStandardDeviations;

/** PointVariances

  Container for the variances of the points in a histogram. A copy-on-write
  mechanism saves memory and makes copying cheap. The implementation is based on
  detail::VarianceVectorOf, which provides conversion from the corresponding
  standard deviation type, PointStandardDeviations.

  @author Simon Heybrock
  @date 2016

  Copyright &copy; 2016 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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
class MANTID_HISTOGRAMDATA_DLL PointVariances
    : public detail::VarianceVectorOf<PointVariances, HistogramDx,
                                      PointStandardDeviations> {
public:
  using VarianceVectorOf<PointVariances, HistogramDx,
                         PointStandardDeviations>::VarianceVectorOf;
  using VarianceVectorOf<PointVariances, HistogramDx, PointStandardDeviations>::
  operator=;
  /// Default constructor, creates a NULL object.
  PointVariances() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  PointVariances(const PointVariances &) = default;
  /// Move constructor.
  PointVariances(PointVariances &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  PointVariances &operator=(const PointVariances &) & = default;
  /// Move assignment.
  PointVariances &operator=(PointVariances &&) & = default;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_POINTVARIANCES_H_ */
