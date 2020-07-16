// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionStatistics.h"
#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAlgorithms/SampleCorrections/RectangularBeamProfile.h"
#include "MantidDataObjects/Histogram1D.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/Logger.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MonteCarloTesting.h"

using Mantid::Algorithms::MCAbsorptionStrategy;
using Mantid::Algorithms::MCInteractionStatistics;
using Mantid::Algorithms::MCInteractionVolume;

class MCAbsorptionStrategyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MCAbsorptionStrategyTest *createSuite() {
    return new MCAbsorptionStrategyTest();
  }
  static void destroySuite(MCAbsorptionStrategyTest *suite) { delete suite; }

  //----------------------------------------------------------------------------
  // Success cases
  //----------------------------------------------------------------------------
  void test_Simulation_Runs_Over_Expected_Number_Events() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    auto testSampleSphere = MonteCarloTesting::createTestSample(
        MonteCarloTesting::TestSampleType::SolidSphere);
    MockBeamProfile testBeamProfile;
    EXPECT_CALL(testBeamProfile, defineActiveRegion(_))
        .WillOnce(Return(testSampleSphere.getShape().getBoundingBox()));
    const size_t nevents(10), maxTries(100);
    MCAbsorptionStrategy mcabsorb(testBeamProfile, testSampleSphere,
                                  Mantid::Kernel::DeltaEMode::Type::Direct,
                                  nevents, maxTries, false);
    // 3 random numbers per event expected
    MockRNG rng;
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(30))
        .WillRepeatedly(Return(0.5));
    const Mantid::Algorithms::IBeamProfile::Ray testRay = {V3D(-2, 0, 0),
                                                           V3D(1, 0, 0)};
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
    mcabsorb.calculate(rng, endPos, lambdas, lambdaFixed, attenuationFactors,
                       attenuationFactorErrors, trackStatistics);
    TS_ASSERT_DELTA(0.0043828472, attenuationFactors[0], 1e-08);
    TS_ASSERT_DELTA(1.0 / std::sqrt(nevents), attenuationFactorErrors[0],
                    1e-08);
  }

  void test_Calculate() {
    MockBeamProfile testBeamProfile;
    MCAbsorptionStrategyWithMock testStrategy(
        testBeamProfile, Mantid::API::Sample(),
        Mantid::Kernel::DeltaEMode::Elastic, 1, 1, true,
        MCInteractionVolume::ScatteringPointVicinity::SAMPLEONLY);
    EXPECT_CALL(rng, calculateBeforeAfterTrack())
        .Times(Exactly(30))
        .WillRepeatedly(Return(0.5));
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

    auto testThinAnnulus = MonteCarloTesting::createTestSample(
        MonteCarloTesting::TestSampleType::ThinAnnulus);
    RectangularBeamProfile testBeamProfile(
        ReferenceFrame(Y, Z, Right, "source"), V3D(), 1, 1);
    const size_t nevents(10), maxTries(1);
    MCAbsorptionStrategy mcabs(testBeamProfile, testThinAnnulus,
                               Mantid::Kernel::DeltaEMode::Type::Direct,
                               nevents, maxTries, false);
    MockRNG rng;
    EXPECT_CALL(rng, nextValue()).WillRepeatedly(Return(0.5));
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    const double lambdaFixed = lambdaAfter;
    const V3D endPos(0.7, 0.7, 1.4);
    std::vector<double> lambdas = {lambdaBefore};
    std::vector<double> attenuationFactors = {0};
    std::vector<double> attenuationFactorErrors = {0};
    MCInteractionStatistics trackStatistics(-1, testThinAnnulus);
    TS_ASSERT_THROWS(mcabs.calculate(rng, endPos, lambdas, lambdaFixed,
                                     attenuationFactors,
                                     attenuationFactorErrors, trackStatistics),
                     const std::runtime_error &)
  }

private:
  class MockBeamProfile final : public Mantid::Algorithms::IBeamProfile {
  public:
    using Mantid::Algorithms::IBeamProfile::Ray;
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD1(generatePoint,
                       Ray(Mantid::Kernel::PseudoRandomNumberGenerator &));
    MOCK_CONST_METHOD2(generatePoint,
                       Ray(Mantid::Kernel::PseudoRandomNumberGenerator &,
                           const Mantid::Geometry::BoundingBox &));
    MOCK_CONST_METHOD1(defineActiveRegion, Mantid::Geometry::BoundingBox(
                                               const Mantid::API::Sample &));
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };
  class MockMCInteractionVolume final : public MCInteractionVolume {
  public:
    MockMCInteractionVolume(const Mantid::API::Sample &sample,
                            const Mantid::Geometry::BoundingBox &activeRegion)
        : MCInteractionVolume(sample, activeRegion) {}
    GNU_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD6(calculateBeforeAfterTrack,
                       bool(Mantid::Kernel::PseudoRandomNumberGenerator &rng,
                            const Mantid::Kernel::V3D &startPos,
                            const Mantid::Kernel::V3D &endPos,
                            Mantid::Geometry::Track &beforeScatter,
                            Mantid::Geometry::Track &afterScatter,
                            MCInteractionStatistics &stats));
    MOCK_CONST_METHOD4(calculateAbsorption,
                       double(const Mantid::Geometry::Track &beforeScatter,
                              const Mantid::Geometry::Track &afterScatter,
                              double lambdaBefore, double lambdaAfter));
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };
  class MCAbsorptionStrategyWithMock final : public MCAbsorptionStrategy {
  public:
    MCAbsorptionStrategyWithMock(
        const Mantid::Algorithms::IBeamProfile &beamProfile,
        const Mantid::API::Sample &sample,
        Mantid::Kernel::DeltaEMode::Type EMode, const size_t nevents,
        const size_t maxScatterPtAttempts,
        const bool regenerateTracksForEachLambda,
        const MCInteractionVolume::ScatteringPointVicinity pointsIn)
        : MCAbsorptionStrategy(beamProfile, sample, EMode, nevents,
                               maxScatterPtAttempts,
                               regenerateTracksForEachLambda, pointsIn){};

  protected:
    MCInteractionVolume createMCInteractionVolume(
        const Mantid::API::Sample &sample,
        const Mantid::Geometry::BoundingBox &activeRegion,
        const size_t maxScatterAttempts,
        const MCInteractionVolume::ScatteringPointVicinity pointsIn) override {
      return MockMCInteractionVolume(sample, activeRegion);
    };
  };
  Mantid::Kernel::Logger g_log{"MCAbsorptionStrategyTest"};
};
