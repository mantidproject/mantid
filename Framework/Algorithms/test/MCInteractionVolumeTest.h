// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_
#define MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidKernel/MersenneTwister.h"
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
  void test_Bounding_Volume_Matches_Sample() {
    using namespace MonteCarloTesting;
    auto sample = createTestSample(TestSampleType::SolidSphere);
    const auto sampleBox = sample.getShape().getBoundingBox();
    MCInteractionVolume interactor(sample, sampleBox);

    const auto interactionBox = interactor.getBoundingBox();
    TS_ASSERT_EQUALS(sampleBox.minPoint(), interactionBox.minPoint());
    TS_ASSERT_EQUALS(sampleBox.maxPoint(), interactionBox.maxPoint());
  }

  void test_Absorption_In_Solid_Sample_Gives_Expected_Answer() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    MockRNG rng;
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3))
        .WillRepeatedly(Return(0.25));

    auto sample = createTestSample(TestSampleType::SolidSphere);
    MCInteractionVolume interactor(sample, sample.getShape().getBoundingBox());
    const double factor = interactor.calculateAbsorption(
        rng, startPos, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.0028357258, factor, 1e-8);
  }

  void test_Absorption_In_Sample_With_Hole_Container_Scatter_In_All_Segments() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    auto sample = createTestSample(TestSampleType::Annulus);

    MockRNG rng;
    // force scatter in segment 1
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3))
        .WillRepeatedly(Return(0.25));

    MCInteractionVolume interactor(sample, sample.getShape().getBoundingBox());
    const double factorSeg1 = interactor.calculateAbsorption(
        rng, startPos, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.030489479, factorSeg1, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);

    // force scatter in segment 2
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3))
        .WillRepeatedly(Return(0.75));
    const double factorSeg2 = interactor.calculateAbsorption(
        rng, startPos, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.033119242, factorSeg2, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);
  }

  void
  test_Absorption_In_Sample_And_Environment_Container_Scatter_In_All_Segments() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);
    const double lambdaBefore(2.5), lambdaAfter(3.5);

    auto sample = createTestSample(TestSampleType::SamplePlusContainer);
    MockRNG rng;
    // force scatter in segment can
    EXPECT_CALL(rng, nextInt(1, 1)).Times(Exactly(1)).WillOnce(Return(1));
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(4))
        .WillOnce(Return(0.75))
        .WillOnce(Return(0.02))
        .WillOnce(Return(0.5))
        .WillOnce(Return(0.5));

    MCInteractionVolume interactor(sample,
                                   sample.getEnvironment().boundingBox());
    const double factorContainer = interactor.calculateAbsorption(
        rng, startPos, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.69223681, factorContainer, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);

    // force scatter in sample
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(4))
        .WillOnce(Return(0.25))
        .WillRepeatedly(Return(0.25));
    const double factorSample = interactor.calculateAbsorption(
        rng, startPos, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.73100698, factorSample, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------
  void test_Construction_With_Invalid_Sample_Shape_Throws_Error() {
    using Mantid::API::Sample;

    Sample sample;
    // nothing
    TS_ASSERT_THROWS(
        MCInteractionVolume mcv(sample, sample.getShape().getBoundingBox()),
        const std::invalid_argument &);
    // valid shape
    sample.setShape(ComponentCreationHelper::createSphere(1));
    TS_ASSERT_THROWS_NOTHING(
        MCInteractionVolume mcv(sample, sample.getShape().getBoundingBox()));
  }

  void test_Throws_If_Point_Cannot_Be_Generated() {
    using namespace Mantid::Kernel;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);
    const double lambdaBefore(2.5), lambdaAfter(3.5);

    auto sample = createTestSample(TestSampleType::ThinAnnulus);
    MersenneTwister rng;
    rng.setSeed(1);
    const size_t maxTries(1);
    MCInteractionVolume interactor(sample, sample.getShape().getBoundingBox(),
                                   maxTries);
    TS_ASSERT_THROWS(interactor.calculateAbsorption(rng, startPos, endPos,
                                                    lambdaBefore, lambdaAfter),
                     const std::runtime_error &);
  }
};

#endif /* MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_ */
