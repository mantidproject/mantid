#ifndef MANTID_KERNEL_HISTOGRAM_H_
#define MANTID_KERNEL_HISTOGRAM_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/cow_ptr.h"
#include "MantidKernel/Histogram/BinEdges.h"
#include "MantidKernel/Histogram/Points.h"

#include <vector>

namespace Mantid {
namespace Kernel {

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
class MANTID_KERNEL_DLL Histogram {
private:
  // MantidVecPtr refX;
  BinEdges m_binEdges;
  Points m_points;

public:
  enum class XMode { BinEdges, Points, Any, Uninitialized };
  Histogram(XMode mode) : m_xMode(mode) {
    switch (mode) {
    case XMode::BinEdges:
      m_binEdges = BinEdges(0);
      break;
    case XMode::Points:
      m_points = Points(0);
      break;
    default:
      throw std::logic_error("Histogram: XMode must be BinEdges or Points");
    }
  }

  XMode xMode() const noexcept { return m_xMode; }
  void setXMode(XMode mode) {
    if (mode != XMode::BinEdges || mode != XMode::Points)
      throw std::logic_error("Histogram: XMode must be BinEdges or Points");
    if (m_xMode == mode)
      return;
    m_xMode = mode;
    if (mode == XMode::Points) {
      m_points = Points(m_binEdges);
      m_binEdges = BinEdges();
    } else {
      m_binEdges = BinEdges(m_points);
      m_points = Points();
    }
  }

  // Temporary legacy interface to X
  void setX(const MantidVec &X) {
    if (xMode() == XMode::BinEdges)
      m_binEdges = X;
    else
      m_points = X;
  }
  void setX(const MantidVecPtr &X) {
    if (xMode() == XMode::BinEdges)
      m_binEdges = X;
    else
      m_points = X;
  }
  void setX(const MantidVecPtr::ptr_type &X) {
    if (xMode() == XMode::BinEdges)
      m_binEdges = X;
    else
      m_points = X;
  }
  MantidVec &dataX() {
    if (xMode() == XMode::BinEdges)
      return m_binEdges.data();
    else
      return m_points.data();
  }
  const MantidVec &dataX() const {
    if (xMode() == XMode::BinEdges)
      return m_binEdges.data();
    else
      return m_points.data();
  }
  const MantidVec &constDataX() const {
    if (xMode() == XMode::BinEdges)
      return m_binEdges.constData();
    else
      return m_points.constData();
  }
  MantidVecPtr ptrX() const {
    if (xMode() == XMode::BinEdges)
      return m_binEdges.cowData();
    else
      return m_points.cowData();
  }

private:
  XMode m_xMode = XMode::Uninitialized;
};

MANTID_KERNEL_DLL Histogram::XMode getHistogramXMode(size_t xLength,
                                                     size_t yLength);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_HISTOGRAM_H_ */
