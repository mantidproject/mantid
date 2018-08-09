#ifndef MANTID_HISTOGRAMDATA_HISTOGRAME_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAME_H_

#include "MantidHistogramData/Addable.h"
#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/Scalable.h"

namespace Mantid {
namespace HistogramData {
class CountStandardDeviations;
class CountVariances;
class FrequencyStandardDeviations;
class FrequencyVariances;
class HistogramE;
namespace detail {
template <class CountStandardDeviations, class HistogramE> class VectorOf;
template <class CountVariances, class HistogramE> class VectorOf;
template <class FrequencyStandardDeviations, class HistogramE> class VectorOf;
template <class FrequencyVariances, class HistogramE> class VectorOf;
} // namespace detail

/** HistogramE

  This class holds e-data of a histogram. The e-data can be variances or
  standard deviations of counts or frequencies. To prevent breaking the
  histogram, the length of HistogramE cannot be changed via the public
  interface.

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
class MANTID_HISTOGRAMDATA_DLL HistogramE
    : public detail::FixedLengthVector<HistogramE>,
      public detail::Addable<HistogramE>,
      public detail::Scalable<HistogramE> {
public:
  using detail::FixedLengthVector<HistogramE>::FixedLengthVector;
  using detail::FixedLengthVector<HistogramE>::operator=;
  HistogramE() = default;
  // The copy and move constructor and assignment are not captured properly
  // by
  // the using declaration above, so we need them here explicitly.
  HistogramE(const HistogramE &) = default;
  HistogramE(HistogramE &&) = default;
  HistogramE &operator=(const HistogramE &) & = default;
  HistogramE &operator=(HistogramE &&) & = default;
  bool operator==(const HistogramE &rhs) const {
    return this->rawData() == rhs.rawData();
  }
  // These classes are friends, such that they can modify the length.
  friend class Histogram;
  friend class detail::VectorOf<CountStandardDeviations, HistogramE>;
  friend class detail::VectorOf<CountVariances, HistogramE>;
  friend class detail::VectorOf<FrequencyStandardDeviations, HistogramE>;
  friend class detail::VectorOf<FrequencyVariances, HistogramE>;
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAME_H_ */
