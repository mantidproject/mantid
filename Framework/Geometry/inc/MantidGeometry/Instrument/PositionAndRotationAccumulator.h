// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2026 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidKernel/Quat.h"
#include "MantidKernel/V3D.h"

#include <cstddef>
#include <map>
#include <string>

namespace Mantid {
namespace Geometry {

/** Collects the position and rotation an instrument definition specifies piecemeal, so that each
 * can be applied once, complete.
 *
 * An instrument definition may set a position one coordinate at a time ("x", "y", "z"), or in
 * spherical form, and a rotation one angle at a time ("rotx", "roty", "rotz"). Neither can be
 * pushed straight into ComponentInfo as it arrives: a partial position would move the component
 * to a nonsense place, and the order the rotation angles arrive in is not guaranteed, while the
 * composition order of the resulting quaternion must be fixed.
 *
 * This replaces the temporary ParameterMap that used to serve the same purpose via
 * ParameterMap::addPositionCoordinate()/addRotationParam(). The behaviour it reproduces:
 *  - a position starts from the component's current position the first time any coordinate is
 *    set, so specifying only "y" leaves x and z alone;
 *  - a rotation starts from zero on every axis, NOT from the component's current rotation;
 *  - the quaternion is always composed X, then Y, then Z, whatever order the angles arrived in;
 *  - a whole position (the spherical form) replaces whatever was accumulated so far.
 *
 * Both stores are keyed by component index and are deliberately sparse: an instrument definition
 * names a handful of components out of an instrument that may hold hundreds of thousands. They
 * are kept separate rather than combined into one entry so that a component which was moved but
 * never rotated does not come back carrying a default rotation, which would overwrite the
 * rotation it already has.
 */
class MANTID_GEOMETRY_DLL PositionAndRotationAccumulator {
public:
  /// Set one coordinate. `currentPosition` seeds the value the first time this component is
  /// touched, and is ignored afterwards. An unrecognised axis is ignored.
  void setCoordinate(const size_t componentIndex, const std::string &axis, const double value,
                     const Kernel::V3D &currentPosition);

  /// Set the whole position at once, discarding any coordinates accumulated so far.
  void setPosition(const size_t componentIndex, const Kernel::V3D &position);

  /// Set one rotation angle, in degrees. Returns false for an unrecognised axis, which the
  /// caller reports -- notably "rot" itself, which is deliberately dropped.
  bool setRotationAngle(const size_t componentIndex, const std::string &axis, const double degrees);

  /// The accumulated position for each component that had any coordinate set.
  std::map<size_t, Kernel::V3D> positions() const;

  /// The composed rotation for each component that had any angle set.
  std::map<size_t, Kernel::Quat> rotations() const;

private:
  struct Position {
    Kernel::V3D position;
    bool seeded{false};
  };
  struct Rotation {
    double x{0.0};
    double y{0.0};
    double z{0.0};
  };
  std::map<size_t, Position> m_positions;
  std::map<size_t, Rotation> m_rotations;
};

} // namespace Geometry
} // namespace Mantid
