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

  // Copy and move need to be declared and defaulted, since we need to have the
  // lvalue reference qualifier on the assignment operators.
  Histogram(const Histogram &) = default;
  Histogram(Histogram &&) = default;
  Histogram &operator=(const Histogram &)& = default;
  Histogram &operator=(Histogram &&)& = default;

  XMode xMode() const noexcept { return m_xMode; }

  BinEdges binEdges() const;
  Points points() const;
  template <typename... T> void setBinEdges(T &&... data) & ;
  template <typename... T> void setPoints(T &&... data) & ;

  const HistogramX &x() const { return *m_x; }
  HistogramX &mutableX() & { return m_x.access(); }

  Kernel::cow_ptr<HistogramX> sharedX() const { return m_x; }
  void setSharedX(const Kernel::cow_ptr<HistogramX> &X) & ;

  // Temporary legacy interface to X
  void setX(const Kernel::cow_ptr<HistogramX> &X) & { m_x = X; }
  MantidVec &dataX() & { return m_x.access().mutableRawData(); }
  const MantidVec &dataX() const & { return m_x->rawData(); }
  const MantidVec &readX() const { return m_x->rawData(); }
  Kernel::cow_ptr<HistogramX> ptrX() const { return m_x; }

private:
  void checkSize(const Points &points) const;
  void checkSize(const BinEdges &binEdges) const;
  template <class... T> bool selfAssignment(const T &...) { return false; }

  XMode m_xMode;
};

/** Sets the Histogram's bin edges.

 Any arguments that can be used for constructing a BinEdges object are allowed,
 however, a size check ensures that the Histogram stays valid, i.e., that x and
 y lengths are consistent. */
template <typename... T> void Histogram::setBinEdges(T &&... data) & {
  BinEdges edges(std::forward<T>(data)...);
  // If there is no data changing the size is ok.
  // if(m_y)
  checkSize(edges);
  if (selfAssignment(data...))
    return;
  m_xMode = XMode::BinEdges;
  m_x = edges.cowData();
}

/** Sets the Histogram's points.

 Any arguments that can be used for constructing a Points object are allowed,
 however, a size check ensures that the Histogram stays valid, i.e., that x and
 y lengths are consistent. */
template <typename... T> void Histogram::setPoints(T &&... data) & {
  Points points(std::forward<T>(data)...);
  // If there is no data changing the size is ok.
  // if(m_y)
  checkSize(points);
  if (selfAssignment(data...))
    return;
  m_xMode = XMode::Points;
  m_x = points.cowData();
}

template <> inline bool Histogram::selfAssignment(const HistogramX &data) {
  return &data == m_x.get();
}

template <>
inline bool Histogram::selfAssignment(const std::vector<double> &data) {
  return &data == &(m_x->rawData());
}

MANTID_HISTOGRAMDATA_DLL Histogram::XMode getHistogramXMode(size_t xLength,
                                                            size_t yLength);

} // namespace HistogramData
} // namespace Mantid

#endif /* MANTID_HISTOGRAMDATA_HISTOGRAM_H_ */
