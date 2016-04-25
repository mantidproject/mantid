#ifndef MANTID_ALGORITHMS_RECTANGULARBEAMPROFILETEST_H_
#define MANTID_ALGORITHMS_RECTANGULARBEAMPROFILETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

#include <gmock/gmock.h>

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
    using namespace ::testing;

    const double width(0.1), height(0.2);
    // Test-frame is non-standard X=beam
    RectangularBeamProfile profile(createTestFrame(), V3D(), width, height);

    MockRNG rng;
    const double rand(0.75);
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(2))
        .WillRepeatedly(Return(rand));
    TS_ASSERT_EQUALS(V3D(0.0, 0.025, 0.05), profile.generatePoint(rng));
  }

  void test_GeneratePoint_Respects_Center() {
    using Mantid::Kernel::V3D;
    using namespace ::testing;

    const double width(0.1), height(0.2);
    const V3D center(1, 2, -3);
    RectangularBeamProfile profile(createTestFrame(), center, width, height);

    MockRNG rng;
    const double rand(0.75);
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(2))
        .WillRepeatedly(Return(rand));
    TS_ASSERT_EQUALS(V3D(1.0, 2.025, -2.95), profile.generatePoint(rng));
  }

  void test_GeneratePoint_Uses_2_Random_Numbers() {
    using Mantid::Kernel::V3D;
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
    TS_ASSERT_EQUALS(V3D(1.0, 1.975, -2.95), profile.generatePoint(rng));
  }

private:
  class MockRNG final : public Mantid::Kernel::PseudoRandomNumberGenerator {
  public:
    MOCK_METHOD0(nextValue, double());
    MOCK_METHOD2(nextValue, double(double, double));
    MOCK_METHOD2(nextInt, int(int, int));
    MOCK_METHOD0(restart, void());
    MOCK_METHOD0(save, void());
    MOCK_METHOD0(restore, void());
    MOCK_METHOD1(setSeed, void(size_t));
    MOCK_METHOD2(setRange, void(const double, const double));
  };

  Mantid::Geometry::ReferenceFrame createTestFrame() {
    using Mantid::Geometry::ReferenceFrame;
    using Mantid::Geometry::PointingAlong;
    using Mantid::Geometry::Handedness;

    return ReferenceFrame(PointingAlong::Z, PointingAlong::X, Handedness::Right,
                          "source");
  }
};

#endif /* MANTID_ALGORITHMS_RECTANGULARBEAMPROFILETEST_H_ */
