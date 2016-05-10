#ifndef MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONS_H_
#define MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONS_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/VectorOf.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/HistogramDx.h"

namespace Mantid {
namespace HistogramData {

class BinEdgeStandardDeviations;

/** PointStandardDeviations : TODO: DESCRIPTION

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
class MANTID_HISTOGRAMDATA_DLL PointStandardDeviations
    : public detail::VectorOf<PointStandardDeviations, HistogramDx>,
      public detail::Iterable<PointStandardDeviations> {
public:
  using VectorOf<PointStandardDeviations, HistogramDx>::VectorOf;
  using VectorOf<PointStandardDeviations, HistogramDx>::operator=;
  PointStandardDeviations() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  PointStandardDeviations(const PointStandardDeviations &) = default;
  PointStandardDeviations(PointStandardDeviations &&) = default;
  PointStandardDeviations &
  operator=(const PointStandardDeviations &)& = default;
  PointStandardDeviations &operator=(PointStandardDeviations &&)& = default;

  /// Constructs PointStandardDeviations from BinEdgeStandardDeviations, where
  /// each point is a bin center.
  PointStandardDeviations(const BinEdgeStandardDeviations &edges);
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONS_H_ */
