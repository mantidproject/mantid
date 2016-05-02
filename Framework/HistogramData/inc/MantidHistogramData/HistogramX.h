#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMX_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMX_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"

namespace Mantid {
namespace HistogramData {
class BinEdges;
class Points;
class HistogramX;
namespace detail {
template <class BinEdges, class HistogramX> class VectorOf;
template <class Points, class HistogramX> class VectorOf;
}

/** HistogramX : TODO: DESCRIPTION

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
class MANTID_HISTOGRAMDATA_DLL HistogramX
    : public detail::FixedLengthVector<HistogramX> {
public:
  using detail::FixedLengthVector<HistogramX>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramX>::operator=;

  friend class Histogram;
  friend class detail::VectorOf<BinEdges, HistogramX>;
  friend class detail::VectorOf<Points, HistogramX>;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMX_H_ */
