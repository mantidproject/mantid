// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidGeometry/DllConfig.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/V3D.h"

#include <optional>

namespace Mantid {
namespace Kernel {
class PseudoRandomNumberGenerator;
}
namespace Geometry {
namespace detail {
class ShapeInfo;
}
class BoundingBox;
class IObject;
namespace RandomPoint {

MANTID_GEOMETRY_DLL Kernel::V3D inCuboid(const detail::ShapeInfo &shapeInfo, Kernel::PseudoRandomNumberGenerator &rng);

MANTID_GEOMETRY_DLL Kernel::V3D inCylinder(const detail::ShapeInfo &shapeInfo,
                                           Kernel::PseudoRandomNumberGenerator &rng);

MANTID_GEOMETRY_DLL Kernel::V3D inHollowCylinder(const detail::ShapeInfo &shapeInfo,
                                                 Kernel::PseudoRandomNumberGenerator &rng);

MANTID_GEOMETRY_DLL Kernel::V3D inSphere(const detail::ShapeInfo &shapeInfo, Kernel::PseudoRandomNumberGenerator &rng);

MANTID_GEOMETRY_DLL std::optional<Kernel::V3D>
inGenericShape(const IObject &object, Kernel::PseudoRandomNumberGenerator &rng, size_t maxAttempts);

MANTID_GEOMETRY_DLL Kernel::V3D localPointInCylinder(const Kernel::V3D &basis, const Kernel::V3D &alongAxis,
                                                     double polarAngle, double radialLength);

template <Kernel::V3D (*T)(const detail::ShapeInfo &, Kernel::PseudoRandomNumberGenerator &)>
std::optional<Kernel::V3D> bounded(const detail::ShapeInfo &shapeInfo, Kernel::PseudoRandomNumberGenerator &rng,
                                   const BoundingBox &box, size_t maxAttempts);

MANTID_GEOMETRY_DLL std::optional<Kernel::V3D> bounded(const IObject &object, Kernel::PseudoRandomNumberGenerator &rng,
                                                       const BoundingBox &box, size_t maxAttempts);

/**
 * Return a random point in a known shape restricted by a bounding box.
 *
 * This could be called with one of the `inCylinder`, `inSphere`,...
 * functions as the template argument.
 * @param shapeInfo a shape info
 * @param rng a random number generator
 * @param box a restricting box
 * @param maxAttempts number of attempts
 * @return a point or none if maxAttempts was exceeded
 */
template <Kernel::V3D (*randomInShape)(const detail::ShapeInfo &, Kernel::PseudoRandomNumberGenerator &)>
std::optional<Kernel::V3D> bounded(const detail::ShapeInfo &shapeInfo, Kernel::PseudoRandomNumberGenerator &rng,
                                   const BoundingBox &box, size_t maxAttempts) {
  std::optional<Kernel::V3D> point;
  for (size_t attempt{0}; attempt < maxAttempts; ++attempt) {
    const Kernel::V3D pt{randomInShape(shapeInfo, rng)};
    if (box.isPointInside(pt)) {
      point = pt;
      break;
    }
  }
  return point;
}

} // namespace RandomPoint
} // namespace Geometry
} // namespace Mantid
