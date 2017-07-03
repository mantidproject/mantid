#ifndef MANTID_ALGORITHMS_MCABSORPTIONSTRATEGYTEST_H_
#define MANTID_ALGORITHMS_MCABSORPTIONSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MonteCarloTesting.h"

using Mantid::Algorithms::MCAbsorptionStrategy;

class MCAbsorptionStrategyTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MCAbsorptionStrategyTest *createSuite() {
    return new MCAbsorptionStrategyTest();
  }
  static void destroySuite(MCAbsorptionStrategyTest *suite) { delete suite; }

  MCAbsorptionStrategyTest()
      : m_nevents(10), m_testBeamProfile(),
        m_testSample(MonteCarloTesting::createTestSample(
            MonteCarloTesting::TestSampleType::SolidSphere)) {}

  //----------------------------------------------------------------------------
  // Success cases
  //----------------------------------------------------------------------------
  void test_Simulation_Runs_Over_Expected_Number_Events() {
    using Mantid::Kernel::V3D;
    using namespace MonteCarloTesting;
    using namespace ::testing;

    MockRNG rng;
    auto mcabsorb = createTestObject();
    // 3 random numbers per event expected
    EXPECT_CALL(rng, nextValue())
        .Times(Exactly(30))
        .WillRepeatedly(Return(0.5));
    const Mantid::Algorithms::IBeamProfile::Ray testRay = {V3D(-2, 0, 0),
                                                           V3D(1, 0, 0)};
    EXPECT_CALL(m_testBeamProfile, generatePoint(_, _))
        .Times(Exactly(static_cast<int>(m_nevents)))
        .WillRepeatedly(Return(testRay));
    const V3D endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);

    double factor(0.0), error(0.0);
    std::tie(factor, error) =
        mcabsorb.calculate(rng, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(0.0043828472, factor, 1e-08);
    TS_ASSERT_DELTA(1.0 / std::sqrt(m_nevents), error, 1e-08);
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------

private:
  class MockBeamProfile final : public Mantid::Algorithms::IBeamProfile {
  public:
    using Mantid::Algorithms::IBeamProfile::Ray;
    GCC_DIAG_OFF_SUGGEST_OVERRIDE
    MOCK_CONST_METHOD1(generatePoint,
                       Ray(Mantid::Kernel::PseudoRandomNumberGenerator &));
    MOCK_CONST_METHOD2(generatePoint,
                       Ray(Mantid::Kernel::PseudoRandomNumberGenerator &,
                           const Mantid::Geometry::BoundingBox &));
    MOCK_CONST_METHOD1(defineActiveRegion, Mantid::Geometry::BoundingBox(
                                               const Mantid::API::Sample &));
    GCC_DIAG_ON_SUGGEST_OVERRIDE
  };

  MCAbsorptionStrategy createTestObject() {
    using namespace ::testing;

    EXPECT_CALL(m_testBeamProfile, defineActiveRegion(_))
        .WillOnce(Return(m_testSample.getShape().getBoundingBox()));
    return MCAbsorptionStrategy(m_testBeamProfile, m_testSample, m_nevents);
  }

  const size_t m_nevents;
  MockBeamProfile m_testBeamProfile;
  Mantid::API::Sample m_testSample;
};

#endif /* MANTID_ALGORITHMS_MCABSORPTIONSTRATEGYTEST_H_ */
