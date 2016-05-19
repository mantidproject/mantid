#ifndef MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONS_H_
#define MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONS_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/StandardDeviationVectorOf.h"
#include "MantidHistogramData/HistogramE.h"

namespace Mantid {
namespace HistogramData {

class BinEdges;
class CountVariances;
class FrequencyStandardDeviations;

/** CountStandardDeviations : TODO: DESCRIPTION

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
class MANTID_HISTOGRAMDATA_DLL CountStandardDeviations
    : public detail::StandardDeviationVectorOf<CountStandardDeviations,
                                               HistogramE, CountVariances> {
public:
  using StandardDeviationVectorOf<CountStandardDeviations, HistogramE,
                                  CountVariances>::StandardDeviationVectorOf;
  using StandardDeviationVectorOf<CountStandardDeviations, HistogramE,
                                  CountVariances>::
  operator=;
  CountStandardDeviations() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  CountStandardDeviations(const CountStandardDeviations &) = default;
  CountStandardDeviations(CountStandardDeviations &&) = default;
  CountStandardDeviations &
  operator=(const CountStandardDeviations &)& = default;
  CountStandardDeviations &operator=(CountStandardDeviations &&)& = default;

  /// Constructs CountStandardDeviations from FrequencyStandardDeviations and
  /// bin width based on BinEdges.
  CountStandardDeviations(const FrequencyStandardDeviations &frequencies,
                          const BinEdges &edges);
  /// Move-constructs CountStandardDeviations from FrequencyStandardDeviations
  /// and bin width based on BinEdges.
  CountStandardDeviations(FrequencyStandardDeviations &&frequencies,
                          const BinEdges &edges);
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_COUNTSTANDARDDEVIATIONS_H_ */
