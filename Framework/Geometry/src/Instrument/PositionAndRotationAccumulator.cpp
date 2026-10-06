// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/Instrument/PositionAndRotationAccumulator.h"

namespace Mantid::Geometry {

void PositionAndRotationAccumulator::setCoordinate(const size_t componentIndex, const std::string &axis,
                                                   const double value, const Kernel::V3D &currentPosition) {
  auto &entry = m_positions[componentIndex];
  if (!entry.seeded) {
    entry.position = currentPosition;
    entry.seeded = true;
  }
  if (axis == "x") {
    entry.position.setX(value);
  } else if (axis == "y") {
    entry.position.setY(value);
  } else if (axis == "z") {
    entry.position.setZ(value);
  }
}

void PositionAndRotationAccumulator::setPosition(const size_t componentIndex, const Kernel::V3D &position) {
  m_positions[componentIndex] = {position, true};
}

bool PositionAndRotationAccumulator::setRotationAngle(const size_t componentIndex, const std::string &axis,
                                                      const double degrees) {
  auto &entry = m_rotations[componentIndex];
  bool recognised = true;
  if (axis == "rotx") {
    entry.x = degrees;
  } else if (axis == "roty") {
    entry.y = degrees;
  } else if (axis == "rotz") {
    entry.z = degrees;
  } else {
    recognised = false;
  }
  return recognised;
}

std::map<size_t, Kernel::V3D> PositionAndRotationAccumulator::positions() const {
  std::map<size_t, Kernel::V3D> result;
  for (const auto &[componentIndex, entry] : m_positions) {
    result.emplace(componentIndex, entry.position);
  }
  return result;
}

std::map<size_t, Kernel::Quat> PositionAndRotationAccumulator::rotations() const {
  std::map<size_t, Kernel::Quat> result;
  for (const auto &[componentIndex, entry] : m_rotations) {
    result.emplace(componentIndex, Kernel::Quat(entry.x, Kernel::V3D(1, 0, 0)) *
                                       Kernel::Quat(entry.y, Kernel::V3D(0, 1, 0)) *
                                       Kernel::Quat(entry.z, Kernel::V3D(0, 0, 1)));
  }
  return result;
}

} // namespace Mantid::Geometry
