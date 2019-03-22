// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidGeometry/RandomPoint.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include <boost/math/special_functions/pow.hpp>

namespace Mantid {
namespace Geometry {
namespace RandomPoint {

/**
 * Return a random point in a cuboid shape.
 * @param shapeInfo cuboid's shape info
 * @param rng a random number generate
 * @return a random point inside the cuboid
 */
Kernel::V3D inCuboid(const detail::ShapeInfo &shapeInfo,
                     Kernel::PseudoRandomNumberGenerator &rng) {
  const auto geometry = shapeInfo.cuboidGeometry();
  const double r1{rng.nextValue()};
  const double r2{rng.nextValue()};
  const double r3{rng.nextValue()};
  const Kernel::V3D basis1{geometry.leftFrontTop - geometry.leftFrontBottom};
  const Kernel::V3D basis2{geometry.leftBackBottom - geometry.leftFrontBottom};
  const Kernel::V3D basis3{geometry.rightFrontBottom -
                           geometry.leftFrontBottom};
  return geometry.leftFrontBottom + (basis1 * r1 + basis2 * r2 + basis3 * r3);
}

/**
 * Return a random point in cylinder.
 * @param shapeInfo cylinder's shape info
 * @param rng a random number generator
 * @return a point
 */
Kernel::V3D inCylinder(const detail::ShapeInfo &shapeInfo,
                       Kernel::PseudoRandomNumberGenerator &rng) {
  using boost::math::pow;
  const auto geometry = shapeInfo.cylinderGeometry();
  const double r1{rng.nextValue()};
  const double r2{rng.nextValue()};
  const double r3{rng.nextValue()};
  const double polar{2. * M_PI * r1};
  // The sqrt is needed for a uniform distribution of points.
  const double r{geometry.radius * std::sqrt(r2)};
  const double z{geometry.height * r3};
  const Kernel::V3D alongAxis{geometry.axis * z};
  const Kernel::V3D &basis1{geometry.axis};
  Mantid::Kernel::V3D basis2{1., 0., 0.};
  if (basis1.X() != 0. && basis1.Z() != 0) {
    const auto inverseXZSumSq = 1. / (pow<2>(basis1.X()) + pow<2>(basis1.Z()));
    basis2.setX(std::sqrt(1. - pow<2>(basis1.X()) * inverseXZSumSq));
    basis2.setZ(basis1.X() * std::sqrt(inverseXZSumSq));
  }
  const Kernel::V3D basis3{basis1.cross_prod(basis2)};
  const Kernel::V3D localPoint{
      ((basis2 * std::cos(polar) + basis3 * std::sin(polar)) * r) + alongAxis};
  return geometry.centreOfBottomBase + localPoint;
}

/**
 * Return a random point in a hollow cylinder
 * @param shapeInfo hollow cylinder's shape info
 * @param rng a random number generator
 * @return a point
 */
Kernel::V3D inHollowCylinder(const detail::ShapeInfo &shapeInfo,
                             Kernel::PseudoRandomNumberGenerator &rng) {
  using boost::math::pow;
  const auto geometry = shapeInfo.hollowCylinderGeometry();
  const double r1{rng.nextValue()};
  const double r2{rng.nextValue()};
  const double r3{rng.nextValue()};
  const double polar{2. * M_PI * r1};
  // We need a random number between the inner radius and outer radius, but also
  // need the square root for a uniform distribution of points
  const double c1 = std::pow(geometry.innerRadius, 2);
  const double c2 = std::pow(geometry.outerRadius, 2);
  const double r{std::sqrt(c1 + (c2 - c1) * r2)};
  const double z{geometry.height * r3};
  const Kernel::V3D alongAxis{geometry.axis * z};
  const Kernel::V3D &basis1{geometry.axis};
  Mantid::Kernel::V3D basis2{1., 0., 0.};
  if (basis1.X() != 0. && basis1.Z() != 0) {
    const auto inverseXZSumSq = 1. / (pow<2>(basis1.X()) + pow<2>(basis1.Z()));
    basis2.setX(std::sqrt(1. - pow<2>(basis1.X()) * inverseXZSumSq));
    basis2.setZ(basis1.X() * std::sqrt(inverseXZSumSq));
  }
  const Kernel::V3D basis3{basis1.cross_prod(basis2)};
  const Kernel::V3D localPoint{
      ((basis2 * std::cos(polar) + basis3 * std::sin(polar)) * r) + alongAxis};
  return geometry.centreOfBottomBase + localPoint;
}

/**
 * Return a random point in sphere.
 * @param shapeInfo sphere's shape info
 * @param rng a random number generator
 * @return a point
 */
Kernel::V3D inSphere(const detail::ShapeInfo &shapeInfo,
                     Kernel::PseudoRandomNumberGenerator &rng) {
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
boost::optional<Kernel::V3D>
inGenericShape(const IObject &object, Kernel::PseudoRandomNumberGenerator &rng,
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
boost::optional<Kernel::V3D> bounded(const IObject &object,
                                     Kernel::PseudoRandomNumberGenerator &rng,
                                     const BoundingBox &box,
                                     size_t maxAttempts) {
  boost::optional<Kernel::V3D> point{boost::none};
  if (box.isNull()) {
    throw std::invalid_argument(
        "Invalid bounding box. Cannot generate random point.");
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
} // namespace RandomPoint
} // namespace Geometry
} // namespace Mantid
