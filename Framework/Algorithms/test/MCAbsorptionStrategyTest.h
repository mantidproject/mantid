// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/SampleCorrections/IMCInteractionVolume.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MonteCarloTesting.h"

using Mantid::Algorithms::IMCInteractionVolume;
using Mantid::Algorithms::MCAbsorptionStrategy;
using Mantid::Algorithms::MCInteractionStatistics;
using Mantid::Algorithms::MCInteractionVolume;

class MCAbsorptionStrategyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MCAbsorptionStrategyTest *createSuite() { return new MCAbsorptionStrategyTest(); }
  static void destroySuite(MCAbsorptionStrategyTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success cases
  //----------------------------------------------------------------------------
  void test_Simulation_Runs_Over_Expected_Number_Events() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    auto testSampleSphere = MonteCarloTesting::createTestSample(MonteCarloTesting::TestSampleType::SolidSphere);
    MockBeamProfile testBeamProfile;
    EXPECT_CALL(testBeamProfile, defineActiveRegion(_)).WillOnce(Return(testSampleSphere.getShape().getBoundingBox()));
    const size_t nevents(10), maxTries(100);
    std::shared_ptr<IMCInteractionVolume> interactionVol = std::make_shared<MCInteractionVolume>(testSampleSphere);
    MCAbsorptionStrategy mcabsorb(interactionVol, testBeamProfile, Mantid::Kernel::DeltaEMode::Type::Direct, nevents,
                                  maxTries, false);
    // 3 random numbers per event expected
    MockRNG rng;
    EXPECT_CALL(rng, nextValue()).Times(Exactly(3 * nevents)).WillRepeatedly(Return(0.5));
    const Mantid::Algorithms::IBeamProfile::Ray testRay = {V3D(-2, 0, 0), V3D(1, 0, 0)};
    EXPECT_CALL(testBeamProfile, generatePoint(_, _))
        .Times(Exactly(static_cast<int>(nevents)))
        .WillRepeatedly(Return(testRay));
    const V3D endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    const double lambdaFixed = lambdaAfter;

    std::vector<double> lambdas = {lambdaBefore};
    std::vector<double> attenuationFactors = {0};
    std::vector<double> attenuationFactorErrors = {0};
    MCInteractionStatistics trackStatistics(-1, testSampleSphere);
    mcabsorb.calculate(rng, endPos, lambdas, lambdaFixed, attenuationFactors, attenuationFactorErrors, trackStatistics);
    TS_ASSERT(Mock::VerifyAndClearExpectations(&rng));
    TS_ASSERT(Mock::VerifyAndClearExpectations(&testBeamProfile));
  }

  void test_mean_and_sd_calculation() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    // set source at 8cm away from origin with 6cm radius sphere so
    // that distance from source to top of sphere is 10cm
    Mantid::API::Sample testSampleSphere;
    auto shape = ComponentCreationHelper::createSphere(0.06);
    shape->setMaterial(Mantid::Kernel::Material(
        "test", Mantid::PhysicalConstants::NeutronAtom(0, 0, 0, 0, 0, 1 /*total scattering xs*/, 0 /*absorption xs*/),
        1));
    testSampleSphere.setShape(shape);

    MockBeamProfile testBeamProfile;
    EXPECT_CALL(testBeamProfile, defineActiveRegion(_)).WillOnce(Return(testSampleSphere.getShape().getBoundingBox()));
    const size_t nevents(3), maxTries(100);
    std::shared_ptr<IMCInteractionVolume> interactionVol = std::make_shared<MCInteractionVolume>(testSampleSphere);
    MCAbsorptionStrategy mcabsorb(interactionVol, testBeamProfile, Mantid::Kernel::DeltaEMode::Type::Direct, nevents,
                                  maxTries, false);
    // 3 random numbers per event expected to generate x,y,z of each scatter pt
    MockRNG rng;
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(3 * nevents))
        .WillOnce(Return(0.5)) // one point at origin
        .WillOnce(Return(0.5))
        .WillOnce(Return(0.5))
        .WillOnce(Return(0.5)) // one point up
        .WillOnce(Return(1))
        .WillOnce(Return(0.5))
        .WillOnce(Return(0.5)) // one point down
        .WillOnce(Return(0))
        .WillOnce(Return(0.5));
    const Mantid::Algorithms::IBeamProfile::Ray testRay = {V3D(0, 0, -0.08), V3D(0, 0, 1)};
    EXPECT_CALL(testBeamProfile, generatePoint(_, _))
        .Times(Exactly(static_cast<int>(nevents)))
        .WillRepeatedly(Return(testRay));
    const V3D endPos(0, 0, 0.08);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    const double lambdaFixed = lambdaAfter;

    std::vector<double> lambdas = {lambdaBefore};
    std::vector<double> attenuationFactors = {0};
    std::vector<double> attenuationFactorErrors = {0};
    MCInteractionStatistics trackStatistics(-1, testSampleSphere);
    mcabsorb.calculate(rng, endPos, lambdas, lambdaFixed, attenuationFactors, attenuationFactorErrors, trackStatistics);
    // the track through the origin should be 6 before and after scatter
    // the longer track lengths that touch top and bottom of sphere should be
    // 2h^2/sqrt(L1^2+h^2) ie 7.2 (before scatter) and 7.2 (after scatter)
    std::vector<double> trackLengths = {2 * 7.2, 2 * 6.0, 2 * 7.2};
    double expectedAverage = 0;
    double expectedVar = 0;
    for (auto trackLength : trackLengths) {
      expectedAverage += (exp(-trackLength));
    }
    expectedAverage = expectedAverage / nevents;
    for (auto trackLength : trackLengths) {
      expectedVar += pow(exp(-trackLength) - expectedAverage, 2);
    }
    expectedVar = expectedVar / (nevents - 1);
    TS_ASSERT_DELTA(expectedAverage, attenuationFactors[0], 1e-08);
    double expectedSD = sqrt(expectedVar);
    TS_ASSERT_DELTA(expectedSD / sqrt(nevents), attenuationFactorErrors[0], 1e-08);
  }

  void test_Calculate() {
    using namespace MonteCarloTesting;
    using namespace ::testing;
    MockBeamProfile testBeamProfile;
    std::shared_ptr<MockMCInteractionVolume> testInteractionVolume = std::make_shared<MockMCInteractionVolume>();
    MCAbsorptionStrategy testStrategy(testInteractionVolume, testBeamProfile, Mantid::Kernel::DeltaEMode::Elastic, 5, 2,
                                      true);
    MockRNG rng;
    std::vector<double> attenuationFactors = {0};
    std::vector<double> attenuationFactorErrors = {0};
    MCInteractionStatistics trackStatistics(-1, Mantid::API::Sample{});
    Mantid::Geometry::BoundingBox emptyBoundingBox;
    EXPECT_CALL(testBeamProfile, generatePoint(_, _)).Times(Exactly(static_cast<int>(6)));
    EXPECT_CALL(*testInteractionVolume, getFullBoundingBox()).Times(1).WillOnce(Return(emptyBoundingBox));

    auto beforeScatter = std::make_shared<MockTrack>();
    auto afterScatter = std::make_shared<MockTrack>();
    auto ReturnTracksAndTrue = [beforeScatter, afterScatter](auto &, auto &, auto &, auto &) {
      return std::tuple{true, beforeScatter, afterScatter};
    };
    auto ReturnNullTracksAndFalse = [beforeScatter, afterScatter](auto &, auto &, auto &, auto &) {
      return std::tuple{false, nullptr, nullptr};
    };
    EXPECT_CALL(*testInteractionVolume, calculateBeforeAfterTrack(_, _, _, _))
        .Times(Exactly(6))
        .WillOnce(Invoke(ReturnTracksAndTrue))
        .WillOnce(Invoke(ReturnNullTracksAndFalse))
        .WillOnce(Invoke(ReturnTracksAndTrue))
        .WillOnce(Invoke(ReturnTracksAndTrue))
        .WillOnce(Invoke(ReturnTracksAndTrue))
        .WillOnce(Invoke(ReturnTracksAndTrue));

    EXPECT_CALL(*beforeScatter, calculateAttenuation(_))
        .Times(Exactly(5))
        .WillOnce(Return(1.0))
        .WillOnce(Return(2.0))
        .WillOnce(Return(3.0))
        .WillOnce(Return(4.0))
        .WillOnce(Return(5.0));
    EXPECT_CALL(*afterScatter, calculateAttenuation(_)).Times(Exactly(5)).WillRepeatedly(Return(1.0));
    testStrategy.calculate(rng, {0., 0., 0.}, {1.0}, 0., attenuationFactors, attenuationFactorErrors, trackStatistics);
    TS_ASSERT_EQUALS(attenuationFactors[0], 3.0);
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------

  void test_thin_object_fails_to_generate_point_in_sample() {
    using Mantid::Algorithms::RectangularBeamProfile;
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    auto testThinAnnulus = MonteCarloTesting::createTestSample(MonteCarloTesting::TestSampleType::ThinAnnulus);
    RectangularBeamProfile testBeamProfile(ReferenceFrame(Y, Z, Right, "source"), V3D(), 1, 1);
    const size_t nevents(10), maxTries(1);
    std::shared_ptr<IMCInteractionVolume> interactionVol = std::make_shared<MCInteractionVolume>(testThinAnnulus);
    MCAbsorptionStrategy mcabs(interactionVol, testBeamProfile, Mantid::Kernel::DeltaEMode::Type::Direct, nevents,
                               maxTries, false);
    MockRNG rng;
    EXPECT_CALL(rng, nextValue()).WillRepeatedly(Return(0.5));
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    const double lambdaFixed = lambdaAfter;
    const V3D endPos(0.7, 0.7, 1.4);
    std::vector<double> lambdas = {lambdaBefore};
    std::vector<double> attenuationFactors = {0};
    std::vector<double> attenuationFactorErrors = {0};
    MCInteractionStatistics trackStatistics(-1, testThinAnnulus);
    TS_ASSERT_THROWS(mcabs.calculate(rng, endPos, lambdas, lambdaFixed, attenuationFactors, attenuationFactorErrors,
                                     trackStatistics),
                     const std::runtime_error &)
  }

private:
  class MockBeamProfile final : public Mantid::Algorithms::IBeamProfile {
  public:
    using Mantid::Algorithms::IBeamProfile::Ray;
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD1(generatePoint, Ray(Mantid::Kernel::PseudoRandomNumberGenerator &));
    MOCK_CONST_METHOD2(generatePoint,
                       Ray(Mantid::Kernel::PseudoRandomNumberGenerator &, const Mantid::Geometry::BoundingBox &));
    MOCK_CONST_METHOD1(defineActiveRegion, Mantid::Geometry::BoundingBox(const Mantid::Geometry::BoundingBox &));
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };
  class MockMCInteractionVolume final : public IMCInteractionVolume {
  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD4(calculateBeforeAfterTrack,
                       Mantid::Algorithms::TrackPair(Mantid::Kernel::PseudoRandomNumberGenerator &rng,
                                                     const Mantid::Kernel::V3D &startPos,
                                                     const Mantid::Kernel::V3D &endPos,
                                                     MCInteractionStatistics &stats));
    MOCK_CONST_METHOD0(getFullBoundingBox, const Mantid::Geometry::BoundingBox());
    MOCK_METHOD1(setActiveRegion, void(const Mantid::Geometry::BoundingBox &));
    MOCK_METHOD1(setGaugeVolume, void(Mantid::Geometry::IObject_sptr gaugeVolume));
    MOCK_CONST_METHOD0(getGaugeVolume, Mantid::Geometry::IObject_sptr());
    MOCK_METHOD0(init, void());
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };
  class MockTrack final : public Mantid::Geometry::Track {
  public:
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD1(calculateAttenuation, double(double));
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };

  Mantid::Kernel::Logger g_log{"MCAbsorptionStrategyTest"};
};
