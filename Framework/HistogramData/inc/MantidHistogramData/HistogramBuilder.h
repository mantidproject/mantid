#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMBUILDER_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMBUILDER_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/Histogram.h"

namespace Mantid {
namespace HistogramData {

/** HistogramBuilder is a helper for constructing Histograms based on "legacy
  style" information such as x-length, y-length, and a "distribution" flag. If
  the actual types of x and y (such as BinEdges and Counts) are known the
  constructor of Histogram should be used instead of the builder.

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
class MANTID_HISTOGRAMDATA_DLL HistogramBuilder {
public:
  /// Sets X information. Can be a length or actual X data.
  template <typename... T> void setX(T &&... data) {
    m_x = Kernel::make_cow<HistogramX>(std::forward<T>(data)...);
  }
  /// Sets Y information. Can be a length or actual Y data.
  template <typename... T> void setY(T &&... data) {
    m_y = Kernel::make_cow<HistogramY>(std::forward<T>(data)...);
  }
  /// Sets E information. Can be a length or actual E data.
  template <typename... T> void setE(T &&... data) {
    m_e = Kernel::make_cow<HistogramE>(std::forward<T>(data)...);
  }
  void setDistribution(bool isDistribution);

  Histogram build() const;

private:
  bool m_isDistribution{false};
  Kernel::cow_ptr<HistogramX> m_x{nullptr};
  Kernel::cow_ptr<HistogramY> m_y{nullptr};
  Kernel::cow_ptr<HistogramE> m_e{nullptr};
};

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMBUILDER_H_ */
