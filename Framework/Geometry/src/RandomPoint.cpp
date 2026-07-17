// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/RandomPoint.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include <boost/math/special_functions/pow.hpp>

namespace Mantid::Geometry::RandomPoint {

/**
 * Return a local point in a cylinder shape
 *
 * @param basis a basis vector
 * @param alongAxis symmetry axis vector of a cylinder
 * @param polarAngle a polar angle (in radians) of a point in a cylinder
 * @param radialLength radial position of point in a cylinder
 * @return a local point inside the cylinder
 */
Kernel::V3D localPointInCylinder(const Kernel::V3D &basis, const Kernel::V3D &alongAxis, double polarAngle,
                                 double radialLength) {
  // Use basis to get a second perpendicular vector to define basis2
  Kernel::V3D basis2;
  if (basis.X() == 0) {
    basis2.setX(1.);
  } else if (basis.Y() == 0) {
    basis2.setY(1.);
  } else if (basis.Z() == 0) {
    basis2.setZ(1.);
  } else {
    basis2.setX(-basis.Y());
    basis2.setY(basis.X());
    basis2.normalize();
  }
  const Kernel::V3D basis3{basis.cross_prod(basis2)};
  const Kernel::V3D localPoint{((basis2 * std::cos(polarAngle) + basis3 * std::sin(polarAngle)) * radialLength) +
                               alongAxis};
  return localPoint;
}

/**
 * Return a random point in a cuboid shape.
 * @param shapeInfo cuboid's shape info
 * @param rng a random number generate
 * @return a random point inside the cuboid
 */
Kernel::V3D inCuboid(const detail::ShapeInfo &shapeInfo, Kernel::PseudoRandomNumberGenerator &rng) {
  const auto geometry = shapeInfo.cuboidGeometry();
  const double r1{rng.nextValue()};
  const double r2{rng.nextValue()};
  const double r3{rng.nextValue()};
  const Kernel::V3D basis1{geometry.leftFrontTop - geometry.leftFrontBottom};
  const Kernel::V3D basis2{geometry.leftBackBottom - geometry.leftFrontBottom};
  const Kernel::V3D basis3{geometry.rightFrontBottom - geometry.leftFrontBottom};
  return geometry.leftFrontBottom + (basis1 * r1 + basis2 * r2 + basis3 * r3);
}

/**
 * Return a random point in cylinder.
 * @param shapeInfo cylinder's shape info
 * @param rng a random number generator
 * @return a point
 */
Kernel::V3D inCylinder(const detail::ShapeInfo &shapeInfo, Kernel::PseudoRandomNumberGenerator &rng) {
  const auto geometry = shapeInfo.cylinderGeometry();
  const double r1{rng.nextValue()};
  const double r2{rng.nextValue()};
  const double r3{rng.nextValue()};
  const double polar{2. * M_PI * r1};
  // The sqrt is needed for a uniform distribution of points.
  const double r{geometry.radius * std::sqrt(r2)};
  const double z{geometry.height * r3};
  const Kernel::V3D alongAxis{geometry.axis * z};
  auto localPoint = localPointInCylinder(geometry.axis, alongAxis, polar, r);
  return localPoint + geometry.centreOfBottomBase;
}

/**
 * Return a random point in a hollow cylinder
 * @param shapeInfo hollow cylinder's shape info
 * @param rng a random number generator
 * @return a point
 */
Kernel::V3D inHollowCylinder(const detail::ShapeInfo &shapeInfo, Kernel::PseudoRandomNumberGenerator &rng) {
  const auto geometry = shapeInfo.hollowCylinderGeometry();
  const double r1{rng.nextValue()};
  const double r2{rng.nextValue()};
  const double r3{rng.nextValue()};
  const double polar{2. * M_PI * r1};
  // We need a random number between the inner radius and outer radius, but also
  // need the square root for a uniform distribution of points
  const double c1 = geometry.innerRadius * geometry.innerRadius;
  const double c2 = geometry.radius * geometry.radius;
  const double r{std::sqrt(c1 + (c2 - c1) * r2)};
  const double z{geometry.height * r3};
  const Kernel::V3D alongAxis{geometry.axis * z};
  auto localPoint = localPointInCylinder(geometry.axis, alongAxis, polar, r);
  return localPoint + geometry.centreOfBottomBase;
}

/**
 * Return a random point in sphere.
 * @param shapeInfo sphere's shape info
 * @param rng a random number generator
 * @return a point
 */
Kernel::V3D inSphere(const detail::ShapeInfo &shapeInfo, Kernel::PseudoRandomNumberGenerator &rng) {
  const auto geometry = shapeInfo.sphereGeometry();
  const double r1{rng.nextValue()};
  const double r2{rng.nextValue()};
  const double r3{rng.nextValue()};
  const double azimuthal{2. * M_PI * r1};
  // The acos is needed for a uniform distribution of points.
  const double polar{std::acos(2. * r2 - 1.)};
  const double r{r3 * geometry.radius};
  const double x{r * std::cos(azimuthal) * std::sin(polar)};
  const double y{r * std::sin(azimuthal) * std::sin(polar)};
  const double z{r * std::cos(polar)};
  return geometry.centre + Kernel::V3D{x, y, z};
}

/**
 * Return a random point in a generic shape.
 * @param object an IObject within which the point is generated
 * @param rng a random number generator
 * @param maxAttempts maximum number of random numbers to use before giving up
 * @return a point
 */
std::optional<Kernel::V3D> inGenericShape(const IObject &object, Kernel::PseudoRandomNumberGenerator &rng,
                                          size_t maxAttempts) {
  return bounded(object, rng, object.getBoundingBox(), maxAttempts);
}

/**
 * Return a random point in a generic shape limited by a bounding box.
 * @param object an object in which the point is generated
 * @param rng a random number generator
 * @param box a box restricting the point's volume
 * @param maxAttempts number of attempts to find a suitable point
 * @return a point or none if maxAttempts was exceeded
 */
std::optional<Kernel::V3D> bounded(const IObject &object, Kernel::PseudoRandomNumberGenerator &rng,
                                   const BoundingBox &box, size_t maxAttempts) {
  std::optional<Kernel::V3D> point{std::nullopt};
  if (box.isNull()) {
    throw std::invalid_argument("Invalid bounding box. Cannot generate random point.");
  }
  for (size_t attempts{0}; attempts < maxAttempts; ++attempts) {
    const double r1 = rng.nextValue();
    const double r2 = rng.nextValue();
    const double r3 = rng.nextValue();
    auto pt = box.generatePointInside(r1, r2, r3);
    if (object.isValid(pt)) {
      point = pt;
      break;
    }
  };
  return point;
}
} // namespace Mantid::Geometry::RandomPoint
