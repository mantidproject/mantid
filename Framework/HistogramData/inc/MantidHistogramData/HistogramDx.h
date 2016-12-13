#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMDX_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMDX_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"

namespace Mantid {
namespace HistogramData {
class Histogram;
class PointVariances;
class PointStandardDeviations;
class HistogramDx;
namespace detail {
template <class PointVariances, class HistogramDx> class VectorOf;
template <class PointStandardDeviations, class HistogramDx> class VectorOf;
}

/** HistogramDx

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
class MANTID_HISTOGRAMDATA_DLL HistogramDx
    : public detail::FixedLengthVector<HistogramDx> {
public:
  using detail::FixedLengthVector<HistogramDx>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramDx>::operator=;
  HistogramDx() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  HistogramDx(const HistogramDx &) = default;
  HistogramDx(HistogramDx &&) = default;
  HistogramDx &operator=(const HistogramDx &)& = default;
  HistogramDx &operator=(HistogramDx &&)& = default;

  // These classes are friends, such that they can modify the length.
  friend class Histogram;
  friend class detail::VectorOf<PointVariances, HistogramDx>;
  friend class detail::VectorOf<PointStandardDeviations, HistogramDx>;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMDX_H_ */
