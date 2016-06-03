#ifndef MANTID_ALGORITHMS_MCABSORPTIONSTRATEGYTEST_H_
#define MANTID_ALGORITHMS_MCABSORPTIONSTRATEGYTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MCAbsorptionStrategy.h"
#include "MantidAlgorithms/SampleCorrections/IBeamProfile.h"
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
    // Expectations
    Sequence rand;
    const double step = static_cast<double>(1) / static_cast<double>(m_nevents);
    const double start = step;
    for (size_t i = 0; i < m_nevents; ++i) {
      double next = start + static_cast<double>(i) * step;
      EXPECT_CALL(rng, nextValue()).InSequence(rand).WillOnce(Return(next));
    }
    const Mantid::Algorithms::IBeamProfile::Ray testRay = {V3D(-2, 0, 0),
                                                           V3D(1, 0, 0)};
    EXPECT_CALL(m_testBeamProfile, generatePoint(_))
        .Times(Exactly(static_cast<int>(m_nevents)))
        .WillRepeatedly(Return(testRay));
    const V3D endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);

    double factor(0.0), error(0.0);
    std::tie(factor, error) =
        mcabsorb.calculate(rng, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(8.05621154e-03, factor, 1e-08);
    TS_ASSERT_DELTA(1.0 / std::sqrt(m_nevents), error, 1e-08);
  }

  //----------------------------------------------------------------------------
  // Failure cases
  //----------------------------------------------------------------------------

private:
  class MockBeamProfile final : public Mantid::Algorithms::IBeamProfile {
  public:
    using Mantid::Algorithms::IBeamProfile::Ray;
    MOCK_CONST_METHOD1(generatePoint,
                       Ray(Mantid::Kernel::PseudoRandomNumberGenerator &));
  };

  MCAbsorptionStrategy createTestObject() {
    return MCAbsorptionStrategy(m_testBeamProfile, m_testSample, m_nevents);
  }

  const size_t m_nevents;
  MockBeamProfile m_testBeamProfile;
  Mantid::API::Sample m_testSample;
};

#endif /* MANTID_ALGORITHMS_MCABSORPTIONSTRATEGYTEST_H_ */
