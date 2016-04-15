#ifndef MANTID_KERNEL_HISTOGRAM_HISTOGRAMX_H_
#define MANTID_KERNEL_HISTOGRAM_HISTOGRAMX_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Histogram/HistogramData.h"
#include "MantidKernel/Histogram/ConstIterable.h"
#include "MantidKernel/Histogram/Points.h"
#include "MantidKernel/Histogram/BinEdges.h"

namespace Mantid {
namespace Kernel {

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
class MANTID_KERNEL_DLL HistogramX : public HistogramData<HistogramX> {
private:
  enum class XMode { BinEdges, Points };
  XMode m_xMode;

public:
  explicit HistogramX(const Points &points);
  explicit HistogramX(const BinEdges &binEdges);

  Points points() const {
    if (xMode() == XMode::BinEdges)
      return Points(BinEdges(m_data));
    else
      return Points(m_data);
  }

  template <typename T> void setPoints(T &&data) {
    Points &&points = Points(std::forward<T>(data));
    checkSize(points);
    m_xMode = XMode::Points;
    m_data = points.cowData();
  }

  BinEdges binEdges() const {
    if (xMode() == XMode::Points)
      return BinEdges(Points(m_data));
    else
      return BinEdges(m_data);
  }

  template <typename T> void setBinEdges(T &&data) {
    BinEdges &&edges = BinEdges(std::forward<T>(data));
    checkSize(edges);
    m_xMode = XMode::BinEdges;
    m_data = edges.cowData();
  }

private:
  XMode xMode() const { return m_xMode; }

  void checkSize(const Points &points) const {
    size_t target = size();
    // 0 edges -> 0 points, otherwise points are 1 less than edges.
    if (xMode() == XMode::BinEdges && target > 0)
      target--;
    if (target != points.size())
      throw std::logic_error("HistogramX: size mismatch of Points\n");
  }

  void checkSize(const BinEdges &binEdges) const {
    size_t target = size();
    // 0 points -> 0 edges, otherwise edges are 1 more than points.
    if (xMode() == XMode::Points && target > 0)
      target++;
    if (target != binEdges.size())
      throw std::logic_error("HistogramX: size mismatch of BinEdges\n");
  }
};

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_HISTOGRAM_HISTOGRAMX_H_ */
