#ifndef MANTID_ALGORITHMS_RECTANGULARBEAMPROFILETEST_H_
#define MANTID_ALGORITHMS_RECTANGULARBEAMPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"

#include "MonteCarloTesting.h"

using Mantid::Algorithms::RectangularBeamProfile;

class RectangularBeamProfileTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static RectangularBeamProfileTest *createSuite() {
    return new RectangularBeamProfileTest();
  }
  static void destroySuite(RectangularBeamProfileTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success cases
  //----------------------------------------------------------------------------
  void test_GeneratePoint_Respects_ReferenceFrame() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    const double width(0.1), height(0.2);
    // Test-frame is non-standard X=beam
    RectangularBeamProfile profile(createTestFrame(), V3D(), width, height);

    MockRNG rng;
    const double rand(0.75);
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(2))
        .WillRepeatedly(Return(rand));
    auto ray = profile.generatePoint(rng);
    TS_ASSERT_EQUALS(V3D(0.0, 0.025, 0.05), ray.startPos);
    TS_ASSERT_EQUALS(V3D(1.0, 0, 0), ray.unitDir);
  }

  void test_GeneratePoint_Respects_Center() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    const double width(0.1), height(0.2);
    const V3D center(1, 2, -3);
    RectangularBeamProfile profile(createTestFrame(), center, width, height);

    MockRNG rng;
    const double rand(0.75);
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(2))
        .WillRepeatedly(Return(rand));
    auto ray = profile.generatePoint(rng);
    TS_ASSERT_EQUALS(V3D(1.0, 2.025, -2.95), ray.startPos);
    TS_ASSERT_EQUALS(V3D(1.0, 0, 0), ray.unitDir);
  }

  void test_GeneratePoint_Uses_2_Different_Random_Numbers() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    const double width(0.1), height(0.2);
    const V3D center(1, 2, -3);
    RectangularBeamProfile profile(createTestFrame(), center, width, height);

    MockRNG rng;
    const double rand1(0.75), rand2(0.25);
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(2))
        .WillOnce(Return(rand1))
        .WillRepeatedly(Return(rand2));
    auto ray = profile.generatePoint(rng);
    TS_ASSERT_EQUALS(V3D(1.0, 1.975, -2.95), ray.startPos);
    TS_ASSERT_EQUALS(V3D(1.0, 0, 0), ray.unitDir);
  }

  void test_DefineActiveRegion_beam_larger_than_sample() {
    using Mantid::API::Sample;
    using Mantid::Kernel::V3D;
    const double width(3.3), height(6.9);
    const V3D center;
    RectangularBeamProfile profile(createTestFrame(), center, width, height);
    Sample testSample;
    testSample.setShape(ComponentCreationHelper::createSphere(0.5));

    auto region = profile.defineActiveRegion(testSample);
    TS_ASSERT(region.isNonNull());
    TS_ASSERT_EQUALS(V3D(-0.5, -0.5, -0.5), region.minPoint());
    TS_ASSERT_EQUALS(V3D(0.5, 0.5, 0.5), region.maxPoint());
  }

  void test_DefineActiveRegion_beam_smaller_than_sample() {
    using Mantid::API::Sample;
    using Mantid::Kernel::V3D;
    const double width(0.1), height(0.2);
    const V3D center;
    RectangularBeamProfile profile(createTestFrame(), center, width, height);
    Sample testSample;
    testSample.setShape(ComponentCreationHelper::createSphere(0.5));

    auto region = profile.defineActiveRegion(testSample);
    TS_ASSERT(region.isNonNull());
    TS_ASSERT_EQUALS(V3D(-0.5, -0.05, -0.1), region.minPoint());
    TS_ASSERT_EQUALS(V3D(0.5, 0.05, 0.1), region.maxPoint());
  }

private:
  Mantid::Geometry::ReferenceFrame createTestFrame() {
    using Mantid::Geometry::Handedness;
    using Mantid::Geometry::PointingAlong;
    using Mantid::Geometry::ReferenceFrame;
    // up = Z, beam = X
    return ReferenceFrame(PointingAlong::Z, PointingAlong::X, Handedness::Right,
                          "source");
  }
};

#endif /* MANTID_ALGORITHMS_RECTANGULARBEAMPROFILETEST_H_ */
