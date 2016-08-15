#ifndef MANTID_HISTOGRAMDATA_BINEDGEVARIANCES_H_
#define MANTID_HISTOGRAMDATA_BINEDGEVARIANCES_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/VarianceVectorOf.h"
#include "MantidHistogramData/HistogramDx.h"

namespace Mantid {
namespace HistogramData {

class BinEdgeStandardDeviations;
class PointVariances;

/** BinEdgeVariances : TODO: DESCRIPTION

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
class MANTID_HISTOGRAMDATA_DLL BinEdgeVariances
    : public detail::VarianceVectorOf<BinEdgeVariances, HistogramDx,
                                      BinEdgeStandardDeviations> {
public:
  using VarianceVectorOf<BinEdgeVariances, HistogramDx,
                         BinEdgeStandardDeviations>::VarianceVectorOf;
  using VarianceVectorOf<BinEdgeVariances, HistogramDx,
                         BinEdgeStandardDeviations>::
  operator=;
  BinEdgeVariances() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  BinEdgeVariances(const BinEdgeVariances &) = default;
  BinEdgeVariances(BinEdgeVariances &&) = default;
  BinEdgeVariances &operator=(const BinEdgeVariances &)& = default;
  BinEdgeVariances &operator=(BinEdgeVariances &&)& = default;

  /// Constructs BinEdgeVariances from points, approximating each bin
  /// edge as mid-point between two points.
  explicit BinEdgeVariances(const PointVariances &points);
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_BINEDGEVARIANCES_H_ */
