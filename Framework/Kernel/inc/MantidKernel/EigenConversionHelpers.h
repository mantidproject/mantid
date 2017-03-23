#ifndef MANTID_KERNEL_EIGENCONVERSIONHELPERS_H_
#define MANTID_KERNEL_EIGENCONVERSIONHELPERS_H_

#include "MantidKernel/DllConfig.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

#include <Eigen/Geometry>

namespace Mantid {
namespace Kernel {

/** This header provides conversion helpers between vector and rotation types in
  MantidKernel and equivalent types in `Eigen`.

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

/// Converts Eigen::Vector3d to Kernel::V3D
inline Kernel::V3D toV3D(const Eigen::Vector3d &vec) {
  return Kernel::V3D(vec[0], vec[1], vec[2]);
}

/// Converts Eigen::Quaterniond to Kernel::Quat
inline Kernel::Quat toQuat(const Eigen::Quaterniond &quat) {
  return Kernel::Quat(quat.w(), quat.x(), quat.y(), quat.z());
}

/// Converts Kernel::V3D to Eigen::Vector3d
inline Eigen::Vector3d toVector3d(const Kernel::V3D &vec) {
  return Eigen::Vector3d(vec[0], vec[1], vec[2]);
}

/// Converts Kernel::Quat to Eigen::Quaterniond
inline Eigen::Quaterniond toQuaterniond(const Kernel::Quat &quat) {
  return Eigen::Quaterniond(quat.real(), quat.imagI(), quat.imagJ(),
                            quat.imagK());
}

} // namespace Kernel
} // namespace Mantid

#endif /* MANTID_KERNEL_EIGENCONVERSIONHELPERS_H_ */
