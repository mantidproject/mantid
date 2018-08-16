#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMY_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMY_H_

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/Multipliable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"

namespace Mantid {
namespace HistogramData {
class Counts;
class Frequencies;
class HistogramY;
namespace detail {
template <class Counts, class HistogramY> class VectorOf;
template <class Frequencies, class HistogramY> class VectorOf;
} // namespace detail

/** HistogramY

  This class holds y-data of a histogram. The y-data can be counts or
  frequencies. To prevent breaking the histogram, the length of HistogramY
  cannot be changed via the public interface.

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
class MANTID_HISTOGRAMDATA_DLL HistogramY
    : public detail::FixedLengthVector<HistogramY>,
      public detail::Addable<HistogramY>,
      public detail::Offsetable<HistogramY>,
      public detail::Multipliable<HistogramY>,
      public detail::Scalable<HistogramY> {
public:
  using detail::FixedLengthVector<HistogramY>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramY>::operator=;
  HistogramY() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  HistogramY(const HistogramY &) = default;
  HistogramY(HistogramY &&) = default;
  HistogramY &operator=(const HistogramY &) & = default;
  HistogramY &operator=(HistogramY &&) & = default;
  // Multiple inheritance causes ambiguous overload, bring operators into scope.
  using detail::Addable<HistogramY>::operator+;
  using detail::Addable<HistogramY>::operator+=;
  using detail::Addable<HistogramY>::operator-;
  using detail::Addable<HistogramY>::operator-=;
  using detail::Offsetable<HistogramY>::operator+;
  using detail::Offsetable<HistogramY>::operator+=;
  using detail::Offsetable<HistogramY>::operator-;
  using detail::Offsetable<HistogramY>::operator-=;
  using detail::Multipliable<HistogramY>::operator*=;
  using detail::Multipliable<HistogramY>::operator/=;
  using detail::Scalable<HistogramY>::operator*=;
  using detail::Scalable<HistogramY>::operator/=;

  // These classes are friends, such that they can modify the length.
  friend class Histogram;
  friend class detail::VectorOf<Counts, HistogramY>;
  friend class detail::VectorOf<Frequencies, HistogramY>;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMY_H_ */
