#ifndef MANTID_INDEXING_CONVERSION_H_
#define MANTID_INDEXING_CONVERSION_H_

#include "MantidIndexing/DllConfig.h"
#include "MantidIndexing/GlobalSpectrumIndex.h"
#include "MantidIndexing/SpectrumNumber.h"

namespace Mantid {
namespace Indexing {

/** Conversion helpers from and to (vectors of) integer types such as
  SpectrumNumber and GlobalSpectrumIndex.

  @author Simon Heybrock
  @date 2017

  Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory, NScD Oak Ridge
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

/** Convert std::vector<In> to std::vector<Out> by static-casting all elements.
 *
 * `In` must be an integral type and `Out` can be any type inheriting
 * Indexing::IndexType, such as SpectrumNumber or GlobalSpectrumIndex. The
 * conversion is done using static_cast without any range check. It is the
 * responsibility of the caller to not pass any vectors containing elements that
 * suffer from loss of data in a static_cast. */
template <
    class Out, class In,
    typename std::enable_if<std::is_integral<In>::value>::type * = nullptr>
std::vector<Out> castVector(const std::vector<In> &indices) {
  std::vector<Out> converted;
  converted.reserve(indices.size());
  for (const auto index : indices)
    converted.push_back(static_cast<typename Out::underlying_type>(index));
  return converted;
}

/** Convert std::vector<In> to std::vector<Out> by static-casting all elements.
 *
 * `Out` must be an integral type and `In` can be any type inheriting
 * Indexing::IndexType, such as SpectrumNumber or GlobalSpectrumIndex. The
 * conversion is done using static_cast without any range check. It is the
 * responsibility of the caller to not pass any vectors containing elements that
 * suffer from loss of data in a static_cast. */
template <
    class Out, class In,
    typename std::enable_if<!std::is_integral<In>::value>::type * = nullptr>
std::vector<Out> castVector(const std::vector<In> &indices) {
  std::vector<Out> converted;
  converted.reserve(indices.size());
  for (const auto index : indices)
    converted.push_back(
        static_cast<Out>(static_cast<typename In::underlying_type>(index)));
  return converted;
}

} // namespace Indexing
} // namespace Mantid

#endif /* MANTID_INDEXING_CONVERSION_H_ */
