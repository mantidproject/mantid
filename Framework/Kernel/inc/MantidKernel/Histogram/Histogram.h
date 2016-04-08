#ifndef MANTID_KERNEL_HISTOGRAM_H_
#define MANTID_KERNEL_HISTOGRAM_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/cow_ptr.h"

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
  MantidVecPtr refX;

public:
  enum class XMode { BinEdges, Points, Any, Uninitialized };
  Histogram(XMode mode) : m_xMode(mode) {}

  XMode xMode() const noexcept { return m_xMode; }
  void setXMode(XMode mode) noexcept { m_xMode = mode; }

  // Temporary legacy interface to X
  void setX(const MantidVec &X) { refX.access() = X; }
  void setX(const MantidVecPtr &X) { refX = X; }
  void setX(const MantidVecPtr::ptr_type &X) { refX = X; }
  MantidVec &dataX() { return refX.access(); }
  const MantidVec &dataX() const { return *refX; }
  const MantidVec &constDataX() const { return *refX; }
  MantidVecPtr ptrX() const { return refX; }

private:
  XMode m_xMode = XMode::Uninitialized;
};

MANTID_KERNEL_DLL Histogram::XMode getHistogramXMode(size_t xLength,
                                                     size_t yLength);

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_HISTOGRAM_H_ */
