#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMX_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMX_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"

namespace Mantid {
namespace HistogramData {
class BinEdges;
class Points;
class HistogramX;
namespace detail {
template <class BinEdges, class HistogramX> class VectorOf;
template <class Points, class HistogramX> class VectorOf;
} // namespace detail

/** HistogramX

  This class holds x-data of a histogram. The x-data can be bin edges or points.
  HistogramX is used to directly reference data in a Histogram when it needs to
  be modified without a specific need for bin edges or points. To prevent
  breaking the histogram, the length of HistogramX cannot be changed via the
  public interface.

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
class MANTID_HISTOGRAMDATA_DLL HistogramX
    : public detail::FixedLengthVector<HistogramX>,
      public detail::Offsetable<HistogramX>,
      public detail::Scalable<HistogramX> {
public:
  using detail::FixedLengthVector<HistogramX>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramX>::operator=;
  HistogramX() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  HistogramX(const HistogramX &) = default;
  HistogramX(HistogramX &&) = default;
  HistogramX &operator=(const HistogramX &) & = default;
  HistogramX &operator=(HistogramX &&) & = default;
  bool operator==(const HistogramX &rhs) const {
    return this->rawData() == rhs.rawData();
  }
  // These classes are friends, such that they can modify the length.
  friend class Histogram;
  friend class detail::VectorOf<BinEdges, HistogramX>;
  friend class detail::VectorOf<Points, HistogramX>;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMX_H_ */
