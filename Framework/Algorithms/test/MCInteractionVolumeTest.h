// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/MersenneTwister.h"
#include "MonteCarloTesting.h"

#include <gmock/gmock.h>

using Mantid::Algorithms::ComponentScatterPoint;
using Mantid::Algorithms::IMCInteractionVolume;
using Mantid::Algorithms::MCInteractionStatistics;
using Mantid::Algorithms::MCInteractionVolume;
using namespace ::testing;
using namespace Mantid::Kernel;
using Mantid::Geometry::Track;
using namespace MonteCarloTesting;

class MCInteractionVolumeTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MCInteractionVolumeTest *createSuite() { return new MCInteractionVolumeTest(); }
  static void destroySuite(MCInteractionVolumeTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success cases
  //----------------------------------------------------------------------------
  void test_Bounding_Volume_Matches_Sample() {
    auto sample = createTestSample(TestSampleType::SolidSphere);
    auto sampleBox = sample.getShape().getBoundingBox();
    std::shared_ptr<IMCInteractionVolume> interactor = MCInteractionVolume::create(sample);
    interactor->setActiveRegion(sampleBox);

    const auto interactionBox = interactor->getFullBoundingBox();
    TS_ASSERT_EQUALS(sampleBox.minPoint(), interactionBox.minPoint());
    TS_ASSERT_EQUALS(sampleBox.maxPoint(), interactionBox.maxPoint());
  }

  void test_Solid_Sample_Gives_Expected_Tracks() {
    using Mantid::Kernel::V3D;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(0.7, 0.7, 1.4);
    MockRNG rng;
    EXPECT_CALL(rng, nextValue()).Times(Exactly(3)).WillRepeatedly(Return(0.25));

    auto sample = createTestSample(TestSampleType::SolidSphere);
    std::shared_ptr<IMCInteractionVolume> interactor = MCInteractionVolume::create(sample);
    MCInteractionStatistics trackStatistics(-1, sample);
    auto [success, beforeScatter, afterScatter] =
        interactor->calculateBeforeAfterTrack(rng, startPos, endPos, trackStatistics);
    TS_ASSERT(success);
    TestGeneratedTracks(startPos, endPos, beforeScatter, afterScatter, sample.getShape());
  }

  void test_Sample_With_Hole_Gives_Expected_Tracks() {
    using Mantid::Kernel::V3D;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);
    auto sample = createTestSample(TestSampleType::Annulus);

    MockRNG rng;
    // force scatter near front
    EXPECT_CALL(rng, nextValue()).Times(Exactly(3)).WillRepeatedly(Return(0.25));

    std::shared_ptr<IMCInteractionVolume> interactor = MCInteractionVolume::create(sample);
    MCInteractionStatistics trackStatistics(-1, sample);
    auto [success, beforeScatter, afterScatter] =
        interactor->calculateBeforeAfterTrack(rng, startPos, endPos, trackStatistics);
    TestGeneratedTracks(startPos, endPos, beforeScatter, afterScatter, sample.getShape());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&rng));

    // force scatter near back (longer distance traversed with before lambda)
    EXPECT_CALL(rng, nextValue()).Times(Exactly(3)).WillRepeatedly(Return(0.75));
    std::tie(success, beforeScatter, afterScatter) =
        interactor->calculateBeforeAfterTrack(rng, startPos, endPos, trackStatistics);
    TestGeneratedTracks(startPos, endPos, beforeScatter, afterScatter, sample.getShape());
  }

  void test_Sample_And_Environment_Can_Scatter_In_All_Segments() {
    using Mantid::Kernel::V3D;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);

    auto sample = createTestSample(TestSampleType::SamplePlusContainer);
    MockRNG rng;
    // force scatter in segment can (0)
    EXPECT_CALL(rng, nextInt(-1, 0)).Times(Exactly(1)).WillOnce(Return(0)); // MCInteractionVolume::generatePoint
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3))
        .WillOnce(Return(0.02)) // RandomPoint::bounded r1
        .WillOnce(Return(0.5))  // r2
        .WillOnce(Return(0.5)); // r3

    std::shared_ptr<IMCInteractionVolume> interactor = MCInteractionVolume::create(sample);
    interactor->setActiveRegion(sample.getEnvironment().boundingBox());
    MCInteractionStatistics trackStatistics(-1, sample);
    auto [success, beforeScatter, afterScatter] =
        interactor->calculateBeforeAfterTrack(rng, startPos, endPos, trackStatistics);
    TestGeneratedTracks(startPos, endPos, beforeScatter, afterScatter, sample.getEnvironment().getContainer());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&rng));

    // force scatter in sample (-1)
    EXPECT_CALL(rng, nextInt(-1, 0)).Times(Exactly(1)).WillOnce(Return(-1));      // MCInteractionVolume::generatePoint
    EXPECT_CALL(rng, nextValue()).Times(Exactly(3)).WillRepeatedly(Return(0.25)); // RandomPoint::bounded r1, r2, r3
    std::tie(success, beforeScatter, afterScatter) =
        interactor->calculateBeforeAfterTrack(rng, startPos, endPos, trackStatistics);
    TestGeneratedTracks(startPos, endPos, beforeScatter, afterScatter, sample.getShape());
    TS_ASSERT(Mock::VerifyAndClearExpectations(&rng));
  }

  void test_Construction_With_Env_But_No_Sample_Shape_Does_Not_Throw_Error() {
    Mantid::API::Sample sample;
    auto kit = createTestKit();
    sample.setEnvironment(std::make_unique<Mantid::Geometry::SampleEnvironment>(*kit));

    TS_ASSERT_THROWS_NOTHING(MCInteractionVolume::create(sample));
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------
  void test_Construction_With_Invalid_Sample_Shape_Throws_Error() {
    Mantid::API::Sample sample;
    // nothing
    TS_ASSERT_THROWS(MCInteractionVolume::create(sample), const std::invalid_argument &);
    // valid shape
    sample.setShape(ComponentCreationHelper::createSphere(1));
    TS_ASSERT_THROWS_NOTHING(MCInteractionVolume::create(sample));
  }

  void test_Throws_If_Point_Cannot_Be_Generated() {

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), endPos(2.0, 0.0, 0.0);

    auto sample = createTestSample(TestSampleType::ThinAnnulus);
    MersenneTwister rng;
    rng.setSeed(1);
    const size_t maxTries(1);
    std::shared_ptr<IMCInteractionVolume> interactor = MCInteractionVolume::create(sample, maxTries);
    MCInteractionStatistics trackStatistics(-1, sample);
    TS_ASSERT_THROWS(interactor->calculateBeforeAfterTrack(rng, startPos, endPos, trackStatistics),
                     const std::runtime_error &);
  }

  void testGeneratePointConsidersAllComponents() {

    auto kit = createTestKit();
    size_t maxAttempts(1);

    Mantid::API::Sample sample;
    sample.setShape(ComponentCreationHelper::createSphere(1));
    sample.setEnvironment(std::make_unique<Mantid::Geometry::SampleEnvironment>(*kit));

    std::shared_ptr<IMCInteractionVolume> interactor = MCInteractionVolume::create(sample, maxAttempts);
    interactor->setActiveRegion(kit->getComponent(0).getBoundingBox());

    // Generate "random" sequence
    MockRNG rng;
    // Selects first component
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(0));
    EXPECT_CALL(rng, nextValue()).Times(3).WillOnce(Return(0.55)).WillOnce(Return(0.65)).WillOnce(Return(0.70));
    ComponentScatterPoint comp1Point;
    TS_ASSERT_THROWS_NOTHING(comp1Point = interactor->generatePoint(rng));
    TS_ASSERT(kit->isValid(comp1Point.scatterPoint));
    TS_ASSERT_EQUALS(comp1Point.componentIndex, 0);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&rng));

    // Selects second component
    std::shared_ptr<IMCInteractionVolume> interactor2 = MCInteractionVolume::create(sample, maxAttempts);
    interactor2->setActiveRegion(kit->getComponent(1).getBoundingBox());

    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(1));
    EXPECT_CALL(rng, nextValue()).Times(3).WillOnce(Return(0.55)).WillOnce(Return(0.65)).WillOnce(Return(0.70));
    ComponentScatterPoint comp2Point;
    TS_ASSERT_THROWS_NOTHING(comp2Point = interactor2->generatePoint(rng));
    TS_ASSERT(comp2Point.scatterPoint != comp1Point.scatterPoint);
    TS_ASSERT(kit->isValid(comp2Point.scatterPoint));
    TS_ASSERT_EQUALS(comp2Point.componentIndex, 1);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&rng));

    // Selects third component
    std::shared_ptr<IMCInteractionVolume> interactor3 = MCInteractionVolume::create(sample, maxAttempts);
    interactor3->setActiveRegion(kit->getComponent(2).getBoundingBox());
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(2));
    EXPECT_CALL(rng, nextValue()).Times(3).WillOnce(Return(0.55)).WillOnce(Return(0.65)).WillOnce(Return(0.70));
    ComponentScatterPoint comp3Point;
    TS_ASSERT_THROWS_NOTHING(comp3Point = interactor3->generatePoint(rng));
    TS_ASSERT(comp3Point.scatterPoint != comp2Point.scatterPoint);
    TS_ASSERT(comp3Point.scatterPoint != comp1Point.scatterPoint);
    TS_ASSERT(kit->isValid(comp3Point.scatterPoint));
    TS_ASSERT_EQUALS(comp3Point.componentIndex, 2);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&rng));
  }

  void testGeneratePointRespectsActiveRegion() {

    auto kit = createTestKit();
    size_t maxAttempts(1);

    // Generate "random" sequence
    MockRNG rng;
    // Sequence will try to select one of the non-container pieces
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(1));
    EXPECT_CALL(rng, nextValue()).Times(6).WillRepeatedly(Return(0.5));

    using Mantid::API::Sample;
    Sample sample;
    sample.setShape(ComponentCreationHelper::createSphere(1));
    sample.setEnvironment(std::make_unique<Mantid::Geometry::SampleEnvironment>(*kit));

    std::shared_ptr<IMCInteractionVolume> interactor = MCInteractionVolume::create(sample, maxAttempts);
    // Restrict region to can
    TS_ASSERT_THROWS(interactor->generatePoint(rng), const std::runtime_error &);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&rng));
  }

private:
  Mantid::Kernel::Logger g_log{"MCInteractionVolumeTest"};

  void TestGeneratedTracks(const V3D startPos, const V3D endPos, const std::shared_ptr<Track> beforeScatter,
                           const std::shared_ptr<Track> &afterScatter, const Mantid::Geometry::IObject &shape) {
    // check the generated tracks share the same start point (the scatter point)
    TS_ASSERT_EQUALS(beforeScatter->startPoint(), afterScatter->startPoint());
    TS_ASSERT_EQUALS(shape.isValid(beforeScatter->startPoint()), true);
    // check that the generated tracks intersect the supplied startPos and
    // endPos
    const V3D scatterToEnd = endPos - afterScatter->startPoint();
    TS_ASSERT_EQUALS(afterScatter->direction(), Mantid::Kernel::normalize(scatterToEnd));
    const V3D scatterToStart = startPos - beforeScatter->startPoint();
    TS_ASSERT_EQUALS(beforeScatter->direction(), Mantid::Kernel::normalize(scatterToStart));
  }
};
