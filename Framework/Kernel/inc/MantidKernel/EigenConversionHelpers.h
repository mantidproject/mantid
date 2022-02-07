// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2017 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

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
*/

/// Converts Eigen::Vector3d to Kernel::V3D
inline Kernel::V3D toV3D(const Eigen::Vector3d &vec) { return Kernel::V3D(vec[0], vec[1], vec[2]); }

/// Converts Eigen::Quaterniond to Kernel::Quat
inline Kernel::Quat toQuat(const Eigen::Quaterniond &quat) {
  return Kernel::Quat(quat.w(), quat.x(), quat.y(), quat.z());
}

/// Converts Kernel::V3D to Eigen::Vector3d
inline Eigen::Vector3d toVector3d(const Kernel::V3D &vec) { return Eigen::Vector3d(vec[0], vec[1], vec[2]); }

/// Converts Kernel::Quat to Eigen::Quaterniond
inline Eigen::Quaterniond toQuaterniond(const Kernel::Quat &quat) {
  return Eigen::Quaterniond(quat.real(), quat.imagI(), quat.imagJ(), quat.imagK());
}

} // namespace Kernel
} // namespace Mantid
