#ifndef MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONS_H_
#define MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONS_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/StandardDeviationVectorOf.h"
#include "MantidHistogramData/HistogramE.h"

namespace Mantid {
namespace HistogramData {

class FrequencyVariances;

/** FrequencyStandardDeviations : TODO: DESCRIPTION

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
                                  FrequencyVariances>::
  operator=;
  FrequencyStandardDeviations() = default;
  // The copy and move constructor and assignment are not captured properly by
  // the using declaration above, so we need them here explicitly.
  FrequencyStandardDeviations(const FrequencyStandardDeviations &) = default;
  FrequencyStandardDeviations(FrequencyStandardDeviations &&) = default;
  FrequencyStandardDeviations &
  operator=(const FrequencyStandardDeviations &)& = default;
  FrequencyStandardDeviations &
  operator=(FrequencyStandardDeviations &&)& = default;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONS_H_ */
