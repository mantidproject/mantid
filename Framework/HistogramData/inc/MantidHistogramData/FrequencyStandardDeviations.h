#ifndef MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONS_H_
#define MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONS_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/StandardDeviationVectorOf.h"

namespace Mantid {
namespace HistogramData {

class BinEdges;
class CountStandardDeviations;
class FrequencyVariances;

/** FrequencyStandardDeviations

  Container for the standard deviations of the frequencies in a histogram. A
  copy-on-write mechanism saves memory and makes copying cheap. The
  implementation is based on detail::StandardDeviationVectorOf, which provides
  conversion from the corresponding variance type, FrequencyVariances.

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
class MANTID_HISTOGRAMDATA_DLL FrequencyStandardDeviations
    : public detail::StandardDeviationVectorOf<FrequencyStandardDeviations,
                                               HistogramE, FrequencyVariances> {
public:
  using StandardDeviationVectorOf<
      FrequencyStandardDeviations, HistogramE,
      FrequencyVariances>::StandardDeviationVectorOf;
  using StandardDeviationVectorOf<FrequencyStandardDeviations, HistogramE,
                                  FrequencyVariances>::operator=;
  /// Default constructor, creates a NULL object.
  FrequencyStandardDeviations() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  /// Copy constructor. Lightweight, internal data will be shared.
  FrequencyStandardDeviations(const FrequencyStandardDeviations &) = default;
  /// Move constructor.
  FrequencyStandardDeviations(FrequencyStandardDeviations &&) = default;
  /// Copy assignment. Lightweight, internal data will be shared.
  FrequencyStandardDeviations &
  operator=(const FrequencyStandardDeviations &) & = default;
  /// Move assignment.
  FrequencyStandardDeviations &
  operator=(FrequencyStandardDeviations &&) & = default;

  FrequencyStandardDeviations(const CountStandardDeviations &counts,
                              const BinEdges &edges);
  FrequencyStandardDeviations(CountStandardDeviations &&counts,
                              const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONS_H_ */
