#ifndef MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_
#define MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MonteCarloTesting.h"

#include <gmock/gmock.h>

using Mantid::Algorithms::MCInteractionVolume;

class MCInteractionVolumeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MCInteractionVolumeTest *createSuite() {
    return new MCInteractionVolumeTest();
  }
  static void destroySuite(MCInteractionVolumeTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success cases
  //----------------------------------------------------------------------------
  void test_Absorption_In_Solid_Sample_Gives_Expected_Answer() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), direc(1.0, 0.0, 0.0),
        endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    MockRNG rng;
    EXPECT_CALL(rng, nextInt(1, 1)).Times(Exactly(0));
    EXPECT_CALL(rng, nextValue()).Times(Exactly(1)).WillOnce(Return(0.25));

    auto sample = createTestSample(TestSampleType::SolidSphere);
    MCInteractionVolume interactor(sample);
    const double factor = interactor.calculateAbsorption(
        rng, startPos, direc, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(1.06797501e-02, factor, 1e-8);
  }

  void test_Absorption_In_Sample_With_Hole_Can_Scatter_In_All_Segments() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), direc(1.0, 0.0, 0.0),
        endPos(2.0, 0.0, 0.0);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    auto sample = createTestSample(TestSampleType::Annulus);

    MockRNG rng;
    // force scatter in segment 1
    EXPECT_CALL(rng, nextInt(1, 2)).Times(Exactly(1)).WillOnce(Return(1));
    EXPECT_CALL(rng, nextValue()).Times(Exactly(1)).WillOnce(Return(0.25));

    MCInteractionVolume interactor(sample);
    const double factorSeg1 = interactor.calculateAbsorption(
        rng, startPos, direc, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(5.35624555e-02, factorSeg1, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);

    // force scatter in segment 2
    EXPECT_CALL(rng, nextInt(1, 2)).Times(Exactly(1)).WillOnce(Return(2));
    EXPECT_CALL(rng, nextValue()).Times(Exactly(1)).WillOnce(Return(0.35));
    const double factorSeg2 = interactor.calculateAbsorption(
        rng, startPos, direc, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(7.30835693e-02, factorSeg2, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);
  }

  void test_Absorption_In_Sample_And_Environment_Can_Scatter_In_All_Segments() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), direc(1.0, 0.0, 0.0),
        endPos(2.0, 0.0, 0.0);
    const double lambdaBefore(2.5), lambdaAfter(3.5);

    auto sample = createTestSample(TestSampleType::SamplePlusCan);
    MockRNG rng;
    // force scatter in segment can
    EXPECT_CALL(rng, nextInt(1, 3)).Times(Exactly(1)).WillOnce(Return(1));
    EXPECT_CALL(rng, nextValue()).Times(Exactly(1)).WillOnce(Return(0.3));

    MCInteractionVolume interactor(sample);
    const double factorCan = interactor.calculateAbsorption(
        rng, startPos, direc, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(6.919239804e-01, factorCan, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);

    // force scatter in sample
    EXPECT_CALL(rng, nextInt(1, 3)).Times(Exactly(1)).WillOnce(Return(2));
    EXPECT_CALL(rng, nextValue()).Times(Exactly(1)).WillOnce(Return(0.35));

    const double factorSample = interactor.calculateAbsorption(
        rng, startPos, direc, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(6.9620991317e-01, factorSample, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);
  }

  void test_Track_With_Zero_Intersections_Returns_Unity_Factor() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), direc(0.0, 1.0, 0.0),
        endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    MockRNG rng;
    EXPECT_CALL(rng, nextValue()).Times(Exactly(0));

    auto sample = createTestSample(TestSampleType::SolidSphere);
    MCInteractionVolume interactor(sample);
    TS_ASSERT_DELTA(1.0,
                    interactor.calculateAbsorption(rng, startPos, direc, endPos,
                                                   lambdaBefore, lambdaAfter),
                    1e-08);
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------
  void test_Construction_With_Invalid_Sample_Shape_Throws_Error() {
    using Mantid::API::Sample;

    Sample sample;
    // nothing
    TS_ASSERT_THROWS(MCInteractionVolume mcv(sample), std::invalid_argument);
    // valid shape
    sample.setShape(*ComponentCreationHelper::createSphere(1));
    TS_ASSERT_THROWS_NOTHING(MCInteractionVolume mcv(sample));
  }

  void test_Construction_With_Invalid_Environment_Throws_Error() {
    using Mantid::API::Sample;
    using Mantid::API::SampleEnvironment;

    Sample sample;
    sample.setShape(*ComponentCreationHelper::createSphere(1));
    sample.setEnvironment(new SampleEnvironment("Empty"));
    TS_ASSERT_THROWS(MCInteractionVolume mcv(sample), std::invalid_argument);
  }
};

#endif /* MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_ */
