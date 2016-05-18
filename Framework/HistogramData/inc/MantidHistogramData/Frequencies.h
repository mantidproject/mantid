#ifndef MANTID_HISTOGRAMDATA_FREQUENCIES_H_
#define MANTID_HISTOGRAMDATA_FREQUENCIES_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/VectorOf.h"
#include "MantidHistogramData/Iterable.h"
#include "MantidHistogramData/Offsetable.h"
#include "MantidHistogramData/Scalable.h"
#include "MantidHistogramData/HistogramY.h"

namespace Mantid {
namespace HistogramData {
class BinEdges;
class Counts;

/** Frequencies

  Container for the frequencies in a histogram.

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
class MANTID_HISTOGRAMDATA_DLL Frequencies
    : public detail::VectorOf<Frequencies, HistogramY>,
      public detail::Iterable<Frequencies>,
      public detail::Offsetable<Frequencies>,
      public detail::Scalable<Frequencies> {
public:
  using VectorOf<Frequencies, HistogramY>::VectorOf;
  using VectorOf<Frequencies, HistogramY>::operator=;
  Frequencies() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  Frequencies(const Frequencies &) = default;
  Frequencies(Frequencies &&) = default;
  Frequencies &operator=(const Frequencies &)& = default;
  Frequencies &operator=(Frequencies &&)& = default;

  /// Constructs Frequencies from Counts and bin width based on BinEdges.
  Frequencies(const Counts &counts, const BinEdges &edges);
  Frequencies(Counts &&counts, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_FREQUENCIES_H_ */
