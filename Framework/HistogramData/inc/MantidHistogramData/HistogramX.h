#ifndef MANTID_HISTOGRAMDATA_HISTOGRAMX_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAMX_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidHistogramData/FixedLengthVector.h"
#include "MantidHistogramData/ConstIterable.h"
#include "MantidHistogramData/Points.h"
#include "MantidHistogramData/BinEdges.h"

namespace Mantid {
namespace HistogramData {

/** HistogramX : TODO: DESCRIPTION

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
class MANTID_HISTOGRAMDATA_DLL HistogramX
    : public FixedLengthVector<HistogramX> {
public:
  explicit HistogramX(const Points &points);
  explicit HistogramX(const BinEdges &binEdges);

  HistogramX &operator=(const HistogramX &rhs);

  Points points() const;
  template <typename T> void setPoints(T &&data);
  BinEdges binEdges() const;
  template <typename T> void setBinEdges(T &&data);

private:
  enum class XMode { BinEdges, Points };

  XMode xMode() const noexcept;
  void checkSize(const Points &points) const;
  void checkSize(const BinEdges &binEdges) const;

  XMode m_xMode;
};

template <typename T> void HistogramX::setPoints(T &&data) {
  Points &&points = Points(std::forward<T>(data));
  checkSize(points);
  m_xMode = XMode::Points;
  m_data = points.cowData();
}

template <typename T> void HistogramX::setBinEdges(T &&data) {
  BinEdges &&edges = BinEdges(std::forward<T>(data));
  checkSize(edges);
  m_xMode = XMode::BinEdges;
  m_data = edges.cowData();
}

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAMX_H_ */
