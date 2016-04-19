#ifndef MANTID_HISTOGRAMDATA_HISTOGRAM_H_
#define MANTID_HISTOGRAMDATA_HISTOGRAM_H_

#include "MantidHistogramData/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Points.h"

#include <vector>

namespace Mantid {
namespace HistogramData {

/** Histogram : TODO: DESCRIPTION

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
class MANTID_HISTOGRAMDATA_DLL Histogram {
private:
  Kernel::cow_ptr<HistogramX> m_x;

public:
  enum class XMode { BinEdges, Points };
  Histogram(XMode mode) : m_x(Kernel::make_cow<HistogramX>(0)), m_xMode(mode) {}
  Histogram(const Points &points)
      : m_x(points.cowData()), m_xMode(XMode::Points) {}
  Histogram(const BinEdges &edges)
      : m_x(edges.cowData()), m_xMode(XMode::BinEdges) {}

  XMode xMode() const noexcept { return m_xMode; }

  Points points() const {
    if (xMode() == XMode::BinEdges)
      return Points(BinEdges(m_x));
    else
      return Points(m_x);
  }

  /*
  template <typename T> void setPoints(T &&data) {
    // TODO size check
    if (xMode() == XMode::BinEdges)
    if (m_binEdges)
      m_binEdges = BinEdges{};
    m_xMode = XMode::Points;
    m_points = std::forward<T>(data);
  }
  */

  // Temporary legacy interface to X
  void setX(const MantidVec &X) { m_x.access() = X; }
  void setX(const Kernel::cow_ptr<HistogramX> &X) { m_x = X; }
  void setX(const boost::shared_ptr<HistogramX> &X) { m_x = X; }
  MantidVec &dataX() { return m_x.access().rawData(); }
  const MantidVec &dataX() const { return m_x->rawData(); }
  const MantidVec &constDataX() const { return m_x->rawData(); }
  Kernel::cow_ptr<HistogramX> ptrX() const { return m_x; }

private:
  XMode m_xMode;
};

MANTID_HISTOGRAMDATA_DLL Histogram::XMode getHistogramXMode(size_t xLength,
                                                            size_t yLength);

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAM_H_ */
