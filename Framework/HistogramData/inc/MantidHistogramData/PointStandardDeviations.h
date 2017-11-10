#ifndef MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONS_H_
#define MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONS_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/StandardDeviationVectorOf.h"
#include "MantidHistogramData/HistogramDx.h"

namespace Mantid {
namespace HistogramData {

class PointVariances;

/** PointStandardDeviations

  Container for the standard deviations of the points in a histogram. A
  copy-on-write mechanism saves memory and makes copying cheap. The
  implementation is based on detail::StandardDeviationVectorOf, which provides
  conversion from the corresponding variance type, PointVariances.

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
class MANTID_HISTOGRAMDATA_DLL PointStandardDeviations
    : public detail::StandardDeviationVectorOf<PointStandardDeviations,
                                               HistogramDx, PointVariances> {
public:
  using StandardDeviationVectorOf<PointStandardDeviations, HistogramDx,
                                  PointVariances>::StandardDeviationVectorOf;
  using StandardDeviationVectorOf<PointStandardDeviations, HistogramDx,
                                  PointVariances>::
  operator=;
  /// Default constructor, creates a NULL object.
  PointStandardDeviations() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  PointStandardDeviations(const PointStandardDeviations &) = default;
  /// Move constructor.
  PointStandardDeviations(PointStandardDeviations &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  PointStandardDeviations &
  operator=(const PointStandardDeviations &)& = default;
  /// Move assignment.
  PointStandardDeviations &operator=(PointStandardDeviations &&)& = default;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONS_H_ */
