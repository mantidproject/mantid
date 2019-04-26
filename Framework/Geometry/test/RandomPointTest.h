// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_RANDOMPOINTTEST_H_
#define MANTID_GEOMETRY_RANDOMPOINTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/RandomPoint.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MockRNG.h"
#include <boost/math/special_functions/pow.hpp>

using namespace Mantid::Geometry;
using namespace Mantid::Geometry::RandomPoint;
using namespace Mantid::Kernel;

class RandomPointTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RandomPointTest *createSuite() { return new RandomPointTest(); }
  static void destroySuite(RandomPointTest *suite) { delete suite; }

  void test_inCuboid() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    constexpr double randX{0.55};
    constexpr double randY{0.65};
    constexpr double randZ{0.70};
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randZ));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randX));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randY));
    constexpr double xLength{0.3};
    constexpr double yLength{0.5};
    constexpr double zLength{0.2};
    auto cuboid =
        ComponentCreationHelper::createCuboid(xLength, yLength, zLength);
    const auto point = inCuboid(cuboid->shapeInfo(), rng);
    constexpr double tolerance{1e-10};
    TS_ASSERT_DELTA(point.X(), xLength - randX * 2. * xLength, tolerance);
    TS_ASSERT_DELTA(point.Y(), -yLength + randY * 2. * yLength, tolerance);
    TS_ASSERT_DELTA(point.Z(), -zLength + randZ * 2. * zLength, tolerance);
  }

  void test_inCylinder() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    constexpr double randT{0.65};
    constexpr double randR{0.55};
    constexpr double randZ{0.70};
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randT));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randR));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randZ));
    constexpr double radius{0.3};
    constexpr double height{0.5};
    const V3D axis{0., 0., 1.};
    const V3D bottomCentre{
        -1.,
        2.,
        -3.,
    };
    auto cylinder = ComponentCreationHelper::createCappedCylinder(
        radius, height, bottomCentre, axis, "cyl");
    V3D point = inCylinder(cylinder->shapeInfo(), rng);
    // Global->cylinder local coordinates
    point -= bottomCentre;
    constexpr double tolerance{1e-10};
    const double polarAngle{2. * M_PI * randT};
    const double radialLength{radius * std::sqrt(randR)};
    const double axisLength{height * randZ};
    TS_ASSERT_DELTA(point.X(), radialLength * std::cos(polarAngle), tolerance);
    TS_ASSERT_DELTA(point.Y(), radialLength * std::sin(polarAngle), tolerance);
    TS_ASSERT_DELTA(point.Z(), axisLength, tolerance);
  }

  void test_inHollowCylinder() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    constexpr double randT{0.65};
    constexpr double randR{0.55};
    constexpr double randZ{0.70};
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randT));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randR));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randZ));
    constexpr double innerRadius{0.3};
    constexpr double outerRadius{0.4};
    constexpr double height{0.5};
    const V3D axis{0., 0., 1.};
    const V3D bottomCentre{
        -1.,
        2.,
        -3.,
    };
    auto hollowCylinder = ComponentCreationHelper::createHollowCylinder(
        innerRadius, outerRadius, height, bottomCentre, axis, "hol-cyl");
    V3D point = inHollowCylinder(hollowCylinder->shapeInfo(), rng);
    // Global->cylinder local coordinates
    point -= bottomCentre;
    constexpr double tolerance{1e-10};
    const double c1 = std::pow(innerRadius, 2);
    const double c2 = std::pow(outerRadius, 2);
    const double radialLength{std::sqrt(c1 + (c2 - c1) * randR)};
    const double axisLength{height * randZ};
    const double polarAngle{2. * M_PI * randT};
    TS_ASSERT_DELTA(point.X(), radialLength * std::cos(polarAngle), tolerance);
    TS_ASSERT_DELTA(point.Y(), radialLength * std::sin(polarAngle), tolerance);
    TS_ASSERT_DELTA(point.Z(), axisLength, tolerance);
  }

  void test_inSphere() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    constexpr double randT{0.65};
    constexpr double randF{0.55};
    constexpr double randR{0.70};
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randT));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randF));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randR));
    constexpr double radius{0.23};
    auto sphere = ComponentCreationHelper::createSphere(radius);
    const V3D point = inSphere(sphere->shapeInfo(), rng);
    // Global->cylinder local coordinates
    constexpr double tolerance{1e-10};
    const double azimuthalAngle{2. * M_PI * randT};
    const double polarAngle{std::acos(2. * randF - 1.)};
    const double r{radius * randR};
    TS_ASSERT_DELTA(point.X(),
                    r * std::cos(azimuthalAngle) * std::sin(polarAngle),
                    tolerance);
    TS_ASSERT_DELTA(point.Y(),
                    r * std::sin(azimuthalAngle) * std::sin(polarAngle),
                    tolerance);
    TS_ASSERT_DELTA(point.Z(), r * std::cos(polarAngle), tolerance);
  }

  void test_inGenericShape() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.9));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.5));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.5));
    // Random sequence set up so as to give point inside hole
    auto shell = ComponentCreationHelper::createHollowShell(0.5, 1.0);
    constexpr size_t maxAttempts{1};
    const auto point = inGenericShape(*shell, rng, maxAttempts);
    TS_ASSERT(point)
    constexpr double tolerance{1e-12};
    TS_ASSERT_DELTA(point->X(), (0.9 - 0.5) / 0.5, tolerance)
    TS_ASSERT_DELTA(point->Y(), 0., tolerance)
    TS_ASSERT_DELTA(point->Z(), 0., tolerance)
  }

  void test_inGenericShape_max_attempts() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.1));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.2));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.3));
    // Random sequence set up so as to give point inside hole
    auto shell = ComponentCreationHelper::createHollowShell(0.5, 1.0);
    constexpr size_t maxAttempts{1};
    const auto point = inGenericShape(*shell, rng, maxAttempts);
    TS_ASSERT(!point)
  }

  void test_bounded_in_known_shape() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    constexpr double randX{0.51};
    constexpr double randY{0.49};
    constexpr double randZ{0.52};
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randZ));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randX));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randY));
    constexpr double xLength{1.};
    constexpr double yLength{1.};
    constexpr double zLength{1.};
    auto cuboid =
        ComponentCreationHelper::createCuboid(xLength, yLength, zLength);
    const BoundingBox box(0.1, 0.1, 0.1, -0.1, -0.1, -0.1);
    constexpr size_t maxAttempts{1};
    const auto point =
        bounded<inCuboid>(cuboid->shapeInfo(), rng, box, maxAttempts);
    TS_ASSERT(point)
    constexpr double tolerance{1e-10};
    TS_ASSERT_DELTA(point->X(), xLength - randX * 2. * xLength, tolerance);
    TS_ASSERT_DELTA(point->Y(), -yLength + randY * 2. * yLength, tolerance);
    TS_ASSERT_DELTA(point->Z(), -zLength + randZ * 2. * zLength, tolerance);
  }

  void test_bounded_in_known_shape_max_attemps() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    constexpr double randX{0.99};
    constexpr double randY{0.99};
    constexpr double randZ{0.99};
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randZ));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randX));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(randY));
    constexpr double xLength{1.};
    constexpr double yLength{1.};
    constexpr double zLength{1.};
    auto cuboid =
        ComponentCreationHelper::createCuboid(xLength, yLength, zLength);
    const BoundingBox box(0.1, 0.1, 0.1, -0.1, -0.1, -0.1);
    constexpr size_t maxAttempts{1};
    const auto point =
        bounded<inCuboid>(cuboid->shapeInfo(), rng, box, maxAttempts);
    TS_ASSERT(!point)
  }

  void test_bounded_in_generic_shape() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.5));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.5));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.5));
    // Random sequence set up so as to give point inside hole
    auto shell = ComponentCreationHelper::createHollowShell(0.5, 1.0);
    const BoundingBox box(1.0, 0.05, 0.05, 0.9, -0.05, -0.05);
    constexpr size_t maxAttempts{1};
    const auto point = bounded(*shell, rng, box, maxAttempts);
    TS_ASSERT(point)
    constexpr double tolerance{1e-12};
    TS_ASSERT_DELTA(point->X(), 0.95, tolerance)
    TS_ASSERT_DELTA(point->Y(), 0., tolerance)
    TS_ASSERT_DELTA(point->Z(), 0., tolerance)
  }

  void test_bounded_in_generic_shape_max_attempts() {
    using namespace ::testing;
    MockRNG rng;
    Sequence rand;
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.5));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.5));
    EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(0.5));
    // Random sequence set up so as to give point inside hole
    auto shell = ComponentCreationHelper::createHollowShell(0.5, 1.0);
    const BoundingBox box(0.1, 0.1, 0.1, -0.1, -0.1, -0.1);
    constexpr size_t maxAttempts{1};
    const auto point = bounded(*shell, rng, box, maxAttempts);
    TS_ASSERT(!point)
  }

  void test_localPointInCylinder() {
    // This test uses a hollow cylinder geometry
    using namespace ::testing;
    constexpr double radialLength{0.3};
    constexpr double polarAngle{0.4};
    const V3D alongAxis{0., 0., 1.};
    const V3D basis{0., 1., 0.};

    const Mantid::Kernel::V3D basis2{1., 0., 0.};
    const Mantid::Kernel::V3D basis3{basis.cross_prod(basis2)};

    auto localPoint =
        localPointInCylinder(basis, alongAxis, polarAngle, radialLength);
    const V3D localPointResult =
        ((basis2 * std::cos(polarAngle) + basis3 * std::sin(polarAngle)) *
         radialLength) +
        alongAxis;
    TS_ASSERT_EQUALS(localPoint, localPointResult);
  }

  void test_localPointInCylinderWithNonzeroXAndZBasisElements() {
    // This test uses a hollow cylinder geometry
    using namespace ::testing;
    using boost::math::pow;
    constexpr double radialLength{0.3};
    constexpr double polarAngle{0.4};
    constexpr V3D alongAxis{0., 0., 1.};
    constexpr V3D basis{1., 1., 1.}; // X and Z elements are not 0 here
    
    Mantid::Kernel::V3D basis2;
    if (basis.X() == 0) {
      Mantid::Kernel::V3D x{1., 0., 0.};
      basis2 = x;
    } else if (basis.Y() == 0) {
      Mantid::Kernel::V3D y{0., 1., 0.};
      basis2 = y;
    } else if (basis.Z() == 0) {
      Mantid::Kernel::V3D z{0., 0., 1.};
      basis2 = z;
    } else {
      Mantid::Kernel::V3D v{-basis.Y(), basis.X(), 0.};
      basis2 = Mantid::Kernel::normalize(v);
    }
    const Mantid::Kernel::V3D basis3{basis.cross_prod(basis2)};

    auto localPoint =
        localPointInCylinder(basis, alongAxis, polarAngle, radialLength);
    const V3D localPointResult =
        ((basis2 * std::cos(polarAngle) + basis3 * std::sin(polarAngle)) *
         radialLength) +
        alongAxis;
    TS_ASSERT_EQUALS(localPoint, localPointResult);
  }
};

#endif /* MANTID_GEOMETRY_RANDOMPOINTTEST_H_ */
