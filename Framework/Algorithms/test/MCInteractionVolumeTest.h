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
#include "MantidKernel/Logger.h"
#include "MantidKernel/MersenneTwister.h"
#include "MonteCarloTesting.h"

#include <gmock/gmock.h>

using Mantid::Algorithms::MCInteractionVolume;
using namespace ::testing;
using namespace Mantid::Kernel;
using namespace MonteCarloTesting;

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
    auto sample = createTestSample(TestSampleType::SolidSphere);
    const auto sampleBox = sample.getShape().getBoundingBox();
    MCInteractionVolume interactor(sample, sampleBox, g_log);

    const auto interactionBox = interactor.getBoundingBox();
    TS_ASSERT_EQUALS(sampleBox.minPoint(), interactionBox.minPoint());
    TS_ASSERT_EQUALS(sampleBox.maxPoint(), interactionBox.maxPoint());
  }

  void test_Absorption_In_Solid_Sample_Gives_Expected_Answer() {
    using Mantid::Kernel::V3D;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    MockRNG rng;
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3))
        .WillRepeatedly(Return(0.25));

    auto sample = createTestSample(TestSampleType::SolidSphere);
    MCInteractionVolume interactor(sample, sample.getShape().getBoundingBox(),
                                   g_log);
    const double factor = interactor.calculateAbsorption(
        rng, startPos, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.0028357258, factor, 1e-8);
  }

  void test_Absorption_In_Sample_With_Hole_Container_Scatter_In_All_Segments() {
    using Mantid::Kernel::V3D;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    auto sample = createTestSample(TestSampleType::Annulus);

    MockRNG rng;
    // force scatter in segment 1
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3))
        .WillRepeatedly(Return(0.25));

    MCInteractionVolume interactor(sample, sample.getShape().getBoundingBox(),
                                   g_log);
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

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);
    const double lambdaBefore(2.5), lambdaAfter(3.5);

    auto sample = createTestSample(TestSampleType::SamplePlusContainer);
    MockRNG rng;
    // force scatter in segment can (1)
    EXPECT_CALL(rng, nextInt(0, 1))
        .Times(Exactly(1))
        .WillOnce(Return(1)); // MCInteractionVolume::generatePoint
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3))
        .WillOnce(Return(0.02)) // RandomPoint::bounded r1
        .WillOnce(Return(0.5))  // r2
        .WillOnce(Return(0.5)); // r3

    MCInteractionVolume interactor(
        sample, sample.getEnvironment().boundingBox(), g_log);
    const double factorContainer = interactor.calculateAbsorption(
        rng, startPos, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.69223681, factorContainer, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);

    // force scatter in sample (0)
    EXPECT_CALL(rng, nextInt(0, 1))
        .Times(Exactly(1))
        .WillOnce(Return(0)); // MCInteractionVolume::generatePoint
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3))
        .WillRepeatedly(Return(0.25)); // RandomPoint::bounded r1, r2, r3
    const double factorSample = interactor.calculateAbsorption(
        rng, startPos, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.73100698, factorSample, 1e-8);
    Mock::VerifyAndClearExpectations(&rng);
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------
  void test_Construction_With_Invalid_Sample_Shape_Throws_Error() {
    Mantid::API::Sample sample;
    // nothing
    TS_ASSERT_THROWS(MCInteractionVolume mcv(
                         sample, sample.getShape().getBoundingBox(), g_log),
                     const std::invalid_argument &);
    // valid shape
    sample.setShape(ComponentCreationHelper::createSphere(1));
    TS_ASSERT_THROWS_NOTHING(MCInteractionVolume mcv(
        sample, sample.getShape().getBoundingBox(), g_log));
  }

  void test_Throws_If_Point_Cannot_Be_Generated() {

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);
    const double lambdaBefore(2.5), lambdaAfter(3.5);

    auto sample = createTestSample(TestSampleType::ThinAnnulus);
    MersenneTwister rng;
    rng.setSeed(1);
    const size_t maxTries(1);
    Mantid::Kernel::Logger g_log("MCInteractionVolumeTest");
    MCInteractionVolume interactor(sample, sample.getShape().getBoundingBox(),
                                   g_log, maxTries);
    TS_ASSERT_THROWS(interactor.calculateAbsorption(rng, startPos, endPos,
                                                    lambdaBefore, lambdaAfter),
                     const std::runtime_error &);
  }

  void testGeneratePointConsidersAllComponents() {

    auto kit = createTestKit();
    size_t maxAttempts(1);

    Mantid::API::Sample sample;
    sample.setShape(ComponentCreationHelper::createSphere(1));
    sample.setEnvironment(
        std::make_unique<Mantid::Geometry::SampleEnvironment>(*kit));

    Mantid::Kernel::Logger g_log("MCInteractionVolumeTest");
    MCInteractionVolume interactor(
        sample, kit->getComponent(0).getBoundingBox(), g_log, maxAttempts);

    // Generate "random" sequence
    MockRNG rng;
    // Selects first component
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(1));
    EXPECT_CALL(rng, nextValue())
        .Times(3)
        .WillOnce(Return(0.55))
        .WillOnce(Return(0.65))
        .WillOnce(Return(0.70));
    V3D comp1Point;
    TS_ASSERT_THROWS_NOTHING(comp1Point = interactor.generatePoint(rng));
    TS_ASSERT(kit->isValid(comp1Point));
    Mock::VerifyAndClearExpectations(&rng);

    // Selects second component
    MCInteractionVolume interactor2(
        sample, kit->getComponent(1).getBoundingBox(), g_log, maxAttempts);

    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(2));
    EXPECT_CALL(rng, nextValue())
        .Times(3)
        .WillOnce(Return(0.55))
        .WillOnce(Return(0.65))
        .WillOnce(Return(0.70));
    V3D comp2Point;
    TS_ASSERT_THROWS_NOTHING(comp2Point = interactor2.generatePoint(rng));
    TS_ASSERT(comp2Point != comp1Point);
    TS_ASSERT(kit->isValid(comp2Point));
    Mock::VerifyAndClearExpectations(&rng);

    // Selects third component
    MCInteractionVolume interactor3(
        sample, kit->getComponent(2).getBoundingBox(), g_log, maxAttempts);
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(3));
    EXPECT_CALL(rng, nextValue())
        .Times(3)
        .WillOnce(Return(0.55))
        .WillOnce(Return(0.65))
        .WillOnce(Return(0.70));
    V3D comp3Point;
    TS_ASSERT_THROWS_NOTHING(comp3Point = interactor3.generatePoint(rng));
    TS_ASSERT(comp3Point != comp2Point);
    TS_ASSERT(comp3Point != comp1Point);
    TS_ASSERT(kit->isValid(comp3Point));
    Mock::VerifyAndClearExpectations(&rng);
  }

  void testGeneratePointRespectsActiveRegion() {

    auto kit = createTestKit();
    size_t maxAttempts(1);

    // Generate "random" sequence
    MockRNG rng;
    // Sequence will try to select one of the non-container pieces
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(2));
    EXPECT_CALL(rng, nextValue()).Times(3).WillRepeatedly(Return(0.5));

    using Mantid::API::Sample;
    Sample sample;
    sample.setShape(ComponentCreationHelper::createSphere(1));
    sample.setEnvironment(
        std::make_unique<Mantid::Geometry::SampleEnvironment>(*kit));

    MCInteractionVolume interactor(sample, kit->getContainer().getBoundingBox(),
                                   g_log, maxAttempts);
    // Restrict region to can
    TS_ASSERT_THROWS(interactor.generatePoint(rng), const std::runtime_error &);
    Mock::VerifyAndClearExpectations(&rng);
  }

private:
  Mantid::Kernel::Logger g_log{"MCInteractionVolumeTest"};
};

#endif /* MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_ */
