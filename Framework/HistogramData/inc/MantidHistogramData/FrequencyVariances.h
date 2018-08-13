#ifndef MANTID_HISTOGRAMDATA_FREQUENCYVARIANCES_H_
#define MANTID_HISTOGRAMDATA_FREQUENCYVARIANCES_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/VarianceVectorOf.h"

namespace Mantid {
namespace HistogramData {

class BinEdges;
class CountVariances;
class FrequencyStandardDeviations;

/** FrequencyVariances

  Container for the variances of the frequencies in a histogram. A copy-on-write
  mechanism saves memory and makes copying cheap. The implementation is based on
  detail::VarianceVectorOf, which provides conversion from the corresponding
  standard deviation type, FrequencyStandardDeviations.

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
class MANTID_HISTOGRAMDATA_DLL FrequencyVariances
    : public detail::VarianceVectorOf<FrequencyVariances, HistogramE,
                                      FrequencyStandardDeviations> {
public:
  using VarianceVectorOf<FrequencyVariances, HistogramE,
                         FrequencyStandardDeviations>::VarianceVectorOf;
  using VarianceVectorOf<FrequencyVariances, HistogramE,
                         FrequencyStandardDeviations>::operator=;
  /// Default constructor, creates a NULL object.
  FrequencyVariances() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  FrequencyVariances(const FrequencyVariances &) = default;
  /// Move constructor.
  FrequencyVariances(FrequencyVariances &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  FrequencyVariances &operator=(const FrequencyVariances &) & = default;
  /// Move assignment.
  FrequencyVariances &operator=(FrequencyVariances &&) & = default;

  FrequencyVariances(const CountVariances &counts, const BinEdges &edges);
  FrequencyVariances(CountVariances &&counts, const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_FREQUENCYVARIANCES_H_ */
