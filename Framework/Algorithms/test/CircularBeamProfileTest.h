// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/SampleCorrections/CircularBeamProfile.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include "MonteCarloTesting.h"

using Mantid::Algorithms::CircularBeamProfile;

class CircularBeamProfileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CircularBeamProfileTest *createSuite() { return new CircularBeamProfileTest(); }
  static void destroySuite(CircularBeamProfileTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success cases
  //----------------------------------------------------------------------------
  void test_GeneratePoint_Respects_ReferenceFrame() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    const double radius(0.5);
    // Test-frame is non-standard X=beam
    CircularBeamProfile profile(createTestFrame(), V3D(), radius);
    // frame, center, radius

    MockRNG rng;
    const double rand(0.5);
    EXPECT_CALL(rng, nextValue()).Times(Exactly(2)).WillRepeatedly(Return(rand));
    auto ray = profile.generatePoint(rng);
    const double testPt3 = -1 * std::sqrt(0.125);
    TS_ASSERT_EQUALS(V3D(0.0, 0.0, testPt3), ray.startPos);
    TS_ASSERT_EQUALS(V3D(1.0, 0, 0), ray.unitDir);
  }

  void test_GeneratePoint_Respects_Center() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    const double radius(0.5);
    const V3D center(2, -3, 1);
    const double testX = 2;
    const double testY = -3;
    const double testZ = 1 - std::sqrt(0.125);
    constexpr const double DBL_EPS = std::numeric_limits<double>::epsilon();
    CircularBeamProfile profile(createTestFrame(), center, radius);

    MockRNG rng;
    const double rand(0.5);
    EXPECT_CALL(rng, nextValue()).Times(Exactly(2)).WillRepeatedly(Return(rand));
    auto ray = profile.generatePoint(rng);
    TS_ASSERT_DELTA(testX, ray.startPos.X(), DBL_EPS);
    TS_ASSERT_DELTA(testY, ray.startPos.Y(), DBL_EPS);
    TS_ASSERT_DELTA(testZ, ray.startPos.Z(), DBL_EPS);
    TS_ASSERT_EQUALS(V3D(1.0, 0, 0), ray.unitDir);
  }

  void test_GeneratePoint_Uses_2_Different_Random_Numbers() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    const double radius(0.5);
    const V3D center;
    // values calculated using polar calculations from V3D class
    const double testX = 0;
    const double testY = sqrt(0.125);
    const double testZ = 0;
    constexpr const double DBL_EPS = std::numeric_limits<double>::epsilon();
    CircularBeamProfile profile(createTestFrame(), center, radius);

    MockRNG rng;
    const double rand1(0.5), rand2(0.25);
    EXPECT_CALL(rng, nextValue()).Times(Exactly(2)).WillOnce(Return(rand1)).WillRepeatedly(Return(rand2));
    auto ray = profile.generatePoint(rng);
    TS_ASSERT_DELTA(testX, ray.startPos.X(), DBL_EPS);
    TS_ASSERT_DELTA(testY, ray.startPos.Y(), DBL_EPS);
    TS_ASSERT_DELTA(testZ, ray.startPos.Z(), DBL_EPS);
    TS_ASSERT_EQUALS(V3D(1.0, 0, 0), ray.unitDir);
  }

  void test_DefineActiveRegion_beam_larger_than_sample() {
    using Mantid::API::Sample;
    using Mantid::Kernel::V3D;
    const double radius(5.0);
    const V3D center;
    CircularBeamProfile profile(createTestFrame(), center, radius);
    Sample testSample;
    testSample.setShape(ComponentCreationHelper::createSphere(0.5));

    auto region = profile.defineActiveRegion(testSample.getShape().getBoundingBox());
    TS_ASSERT(region.isNonNull());
    TS_ASSERT_EQUALS(V3D(-0.5, -0.5, -0.5), region.minPoint());
    TS_ASSERT_EQUALS(V3D(0.5, 0.5, 0.5), region.maxPoint());
  }

  void test_DefineActiveRegion_beam_smaller_than_sample() {
    using Mantid::API::Sample;
    using Mantid::Kernel::V3D;
    const double radius(0.1);
    const V3D center;
    CircularBeamProfile profile(createTestFrame(), center, radius);
    Sample testSample;
    testSample.setShape(ComponentCreationHelper::createSphere(0.5));

    auto region = profile.defineActiveRegion(testSample.getShape().getBoundingBox());
    TS_ASSERT(region.isNonNull());
    TS_ASSERT_EQUALS(V3D(-0.5, -0.1, -0.1), region.minPoint());
    TS_ASSERT_EQUALS(V3D(0.5, 0.1, 0.1), region.maxPoint());
  }

private:
  Mantid::Geometry::ReferenceFrame createTestFrame() {
    using Mantid::Geometry::Handedness;
    using Mantid::Geometry::PointingAlong;
    using Mantid::Geometry::ReferenceFrame;
    // up = Z, beam = X
    return ReferenceFrame(PointingAlong::Z, PointingAlong::X, Handedness::Right, "source");
  }
};
