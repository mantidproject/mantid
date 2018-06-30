#ifndef MANTID_HISTOGRAMDATA_COUNTS_H_
#define MANTID_HISTOGRAMDATA_COUNTS_H_

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"
#include "MantidHistogramData/VectorOf.h"

namespace Mantid {
namespace HistogramData {
class BinEdges;
class Frequencies;

/** Counts

  Container for the counts in a histogram. A copy-on-write mechanism saves
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
class MANTID_HISTOGRAMDATA_DLL Counts
    : public detail::VectorOf<Counts, HistogramY>,
      public detail::Addable<Counts>,
      public detail::Iterable<Counts>,
      public detail::Offsetable<Counts>,
      public detail::Scalable<Counts> {
public:
  using VectorOf<Counts, HistogramY>::VectorOf;
  using VectorOf<Counts, HistogramY>::operator=;
  // Multiple inheritance causes ambiguous overload, bring operators into scope.
  using detail::Addable<Counts>::operator+;
  using detail::Addable<Counts>::operator+=;
  using detail::Addable<Counts>::operator-;
  using detail::Addable<Counts>::operator-=;
  using detail::Offsetable<Counts>::operator+;
  using detail::Offsetable<Counts>::operator+=;
  using detail::Offsetable<Counts>::operator-;
  using detail::Offsetable<Counts>::operator-=;
  /// Default constructor, creates a NULL object.
  Counts() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  Counts(const Counts &) = default;
  /// Move constructor.
  Counts(Counts &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  Counts &operator=(const Counts &) & = default;
  /// Move assignment.
  Counts &operator=(Counts &&) & = default;

  Counts(const Frequencies &frequencies, const BinEdges &edges);
  Counts(Frequencies &&frequencies, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_COUNTS_H_ */
