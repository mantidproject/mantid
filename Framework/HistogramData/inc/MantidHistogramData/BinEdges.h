#ifndef MANTID_HISTOGRAMDATA_BINEDGES_H_
#define MANTID_HISTOGRAMDATA_BINEDGES_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramX.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"
#include "MantidHistogramData/VectorOf.h"

namespace Mantid {
namespace HistogramData {

class Points;

/** BinEdges

  Container for the bin edges of a histogram. A copy-on-write mechanism saves
  memory and makes copying cheap. The implementation is based on
  detail::VectorOf, a wrapper around a cow_ptr. Mixins such as detail::Iterable
  provide iterators and other operations.

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
class MANTID_HISTOGRAMDATA_DLL BinEdges
    : public detail::VectorOf<BinEdges, HistogramX>,
      public detail::Iterable<BinEdges>,
      public detail::Offsetable<BinEdges>,
      public detail::Scalable<BinEdges> {
public:
  using VectorOf<BinEdges, HistogramX>::VectorOf;
  using VectorOf<BinEdges, HistogramX>::operator=;
  /// Default constructor, creates a NULL object.
  BinEdges() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  BinEdges(const BinEdges &) = default;
  /// Move constructor.
  BinEdges(BinEdges &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  BinEdges &operator=(const BinEdges &) & = default;
  /// Move assignment.
  BinEdges &operator=(BinEdges &&) & = default;

  explicit BinEdges(const Points &points);
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_BINEDGES_H_ */
