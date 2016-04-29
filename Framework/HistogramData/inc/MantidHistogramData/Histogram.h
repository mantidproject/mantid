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
  explicit Histogram(XMode mode)
      : m_x(Kernel::make_cow<HistogramX>(0)), m_xMode(mode) {}
  explicit Histogram(const Points &points)
      : m_x(points.cowData()), m_xMode(XMode::Points) {}
  explicit Histogram(const BinEdges &edges)
      : m_x(edges.cowData()), m_xMode(XMode::BinEdges) {
    if (m_x->size() == 1)
      throw std::logic_error("Histogram: BinEdges size cannot be 1");
  }

  XMode xMode() const noexcept { return m_xMode; }

  BinEdges binEdges() const {
    if (xMode() == XMode::BinEdges)
      return BinEdges(m_x);
    else
      return BinEdges(Points(m_x));
  }

  Points points() const {
    if (xMode() == XMode::BinEdges)
      return Points(BinEdges(m_x));
    else
      return Points(m_x);
  }

  template <typename... T> void setBinEdges(T &&... data);
  template <typename... T> void setPoints(T &&... data);

  const HistogramX &x() const { return *m_x; }
  const HistogramX &constX() const { return *m_x; }
  HistogramX &x() { return m_x.access(); }

  // Temporary legacy interface to X
  void setX(const MantidVec &X) { m_x.access().rawData() = X; }
  void setX(const Kernel::cow_ptr<HistogramX> &X) { m_x = X; }
  void setX(const boost::shared_ptr<HistogramX> &X) { m_x = X; }
  MantidVec &dataX() { return m_x.access().rawData(); }
  const MantidVec &dataX() const { return m_x->rawData(); }
  const MantidVec &constDataX() const { return m_x->rawData(); }
  Kernel::cow_ptr<HistogramX> ptrX() const { return m_x; }

private:
  void checkSize(const Points &points) const;
  void checkSize(const BinEdges &binEdges) const;

  XMode m_xMode;
};

template <typename... T> void Histogram::setBinEdges(T &&... data) {
  BinEdges edges(std::forward<T>(data)...);
  // If there is no data changing the size is ok.
  // if(m_y)
  //  checkSize(edges);
  m_xMode = XMode::BinEdges;
  m_x = edges.cowData();
}

template <typename... T> void Histogram::setPoints(T &&... data) {
  Points points(std::forward<T>(data)...);
  // If there is no data changing the size is ok.
  // if(m_y)
  //  checkSize(points);
  m_xMode = XMode::Points;
  m_x = points.cowData();
}

MANTID_HISTOGRAMDATA_DLL Histogram::XMode getHistogramXMode(size_t xLength,
                                                            size_t yLength);

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAM_H_ */
