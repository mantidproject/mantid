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
    MCInteractionVolume interactionVolume(
        testSampleSphere, testBeamProfile.defineActiveRegion(testSampleSphere));
    MCAbsorptionStrategy mcabsorb(interactionVolume, testBeamProfile,
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
    using namespace MonteCarloTesting;
    using namespace ::testing;
    MockBeamProfile testBeamProfile;
    MockMCInteractionVolume testInteractionVolume;
    MCAbsorptionStrategy testStrategy(testInteractionVolume, testBeamProfile,
                                      Mantid::Kernel::DeltaEMode::Elastic, 5, 2,
                                      true);
    MockRNG rng;
    std::vector<double> attenuationFactors = {0};
    std::vector<double> attenuationFactorErrors = {0};
    MCInteractionStatistics trackStatistics(-1, Mantid::API::Sample{});
    EXPECT_CALL(testBeamProfile, generatePoint(_, _))
        .Times(Exactly(static_cast<int>(6)));
    EXPECT_CALL(testInteractionVolume, getBoundingBox())
        .Times(1)
        .WillOnce(ReturnRef(Mantid::Geometry::BoundingBox()));

    EXPECT_CALL(testInteractionVolume,
                calculateBeforeAfterTrack(_, _, _, _, _, _))
        .Times(Exactly(6))
        .WillOnce(Return(true))
        .WillOnce(Return(false))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true))
        .WillOnce(Return(true));
    EXPECT_CALL(testInteractionVolume, calculateAbsorption(_, _, _, _))
        .Times(Exactly(5))
        .WillOnce(Return(1.0))
        .WillOnce(Return(2.0))
        .WillOnce(Return(3.0))
        .WillOnce(Return(4.0))
        .WillOnce(Return(5.0));
    testStrategy.calculate(rng, {0., 0., 0.}, {1.0}, 0., attenuationFactors,
                           attenuationFactorErrors, trackStatistics);
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

    auto testThinAnnulus = MonteCarloTesting::createTestSample(
        MonteCarloTesting::TestSampleType::ThinAnnulus);
    RectangularBeamProfile testBeamProfile(
        ReferenceFrame(Y, Z, Right, "source"), V3D(), 1, 1);
    const size_t nevents(10), maxTries(1);
    MCInteractionVolume interactionVolume(
        testThinAnnulus, testBeamProfile.defineActiveRegion(testThinAnnulus));
    MCAbsorptionStrategy mcabs(interactionVolume, testBeamProfile,
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
  class MockMCInteractionVolume final : public IMCInteractionVolume {
  public:
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
    MOCK_CONST_METHOD0(getBoundingBox, Mantid::Geometry::BoundingBox &());
    GNU_DIAG_ON_SUGGEST_OVERRIDE
  };

  Mantid::Kernel::Logger g_log{"MCAbsorptionStrategyTest"};
};
