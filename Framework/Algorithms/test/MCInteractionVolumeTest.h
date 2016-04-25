#ifndef MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_
#define MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"

#include "MantidTestHelpers/ComponentCreationHelper.h"

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
  void test_Absorption_With_Sample_Only_Gives_Expected_Answer() {
    using Mantid::Kernel::V3D;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), direc(1.0, 0.0, 0.0),
        endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    MockRNG rng;
    EXPECT_CALL(rng, nextValue()).Times(Exactly(1)).WillOnce(Return(0.25));

    auto sample = createTestSample();
    MCInteractionVolume interactor(sample);
    const double factor = interactor.calculateAbsorption(
        rng, startPos, direc, endPos, lambdaBefore, lambdaAfter);
    TS_ASSERT_DELTA(1.06797501e-02, factor, 1e-10);
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

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  class MockRNG final : public Mantid::Kernel::PseudoRandomNumberGenerator {
  public:
    MOCK_METHOD0(nextValue, double());
    MOCK_METHOD0(restart, void());
    MOCK_METHOD0(save, void());
    MOCK_METHOD0(restore, void());
    MOCK_METHOD1(setSeed, void(size_t));
    MOCK_METHOD2(setRange, void(const double, const double));
  };

  Mantid::API::Sample createTestSample() {
    using Mantid::API::Sample;
    using Mantid::Kernel::Material;
    using Mantid::PhysicalConstants::getNeutronAtom;

    Sample testSample;
    auto sphere = ComponentCreationHelper::createSphere(0.1);
    sphere->setMaterial(Material("Vanadium", getNeutronAtom(23), 0.02));
    testSample.setShape(*sphere);
    return testSample;
  }
};

#endif /* MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_ */
