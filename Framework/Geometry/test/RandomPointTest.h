// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_GEOMETRY_RANDOMPOINTTEST_H_
#define MANTID_GEOMETRY_RANDOMPOINTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/RandomPoint.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MockRNG.h"

using namespace Mantid::Geometry::RandomPoint;
using namespace Mantid::Kernel;

class RandomPointTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RandomPointTest *createSuite() { return new RandomPointTest(); }
  static void destroySuite( RandomPointTest *suite ) { delete suite; }


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
    TS_ASSERT_DELTA(xLength - randX * 2. * xLength, point.X(), tolerance);
    TS_ASSERT_DELTA(-yLength + randY * 2. * yLength, point.Y(), tolerance);
    TS_ASSERT_DELTA(-zLength + randZ * 2. * zLength, point.Z(), tolerance);

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
    TS_ASSERT_DELTA(radialLength * std::cos(polarAngle), point.X(), tolerance);
    TS_ASSERT_DELTA(radialLength * std::sin(polarAngle), point.Y(), tolerance);
    TS_ASSERT_DELTA(axisLength, point.Z(), tolerance);
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
    TS_ASSERT_DELTA(r * std::cos(azimuthalAngle) * std::sin(polarAngle),
                    point.X(), tolerance);
    TS_ASSERT_DELTA(r * std::sin(azimuthalAngle) * std::sin(polarAngle),
                    point.Y(), tolerance);
    TS_ASSERT_DELTA(r * std::cos(polarAngle), point.Z(), tolerance);
  }
};

#endif /* MANTID_GEOMETRY_RANDOMPOINTTEST_H_ */
