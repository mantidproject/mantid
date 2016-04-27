#ifndef MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_
#define MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAlgorithms/SampleCorrections/MCInteractionVolume.h"
#include "MantidAPI/Sample.h"
#include "MantidAPI/SampleEnvironment.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
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
  void test_Absorption_In_Solid_Sample_Gives_Expected_Answer() {
    using Mantid::Kernel::V3D;
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

  void test_Track_With_Zero_Intersections_Throws_Error() {
    using Mantid::Kernel::V3D;
    using namespace ::testing;

    // Testing inputs
    const V3D startPos(-2.0, 0.0, 0.0), direc(0.0, 1.0, 0.0),
        endPos(0.7, 0.7, 1.4);
    const double lambdaBefore(2.5), lambdaAfter(3.5);
    MockRNG rng;
    EXPECT_CALL(rng, nextValue()).Times(Exactly(0));

    auto sample = createTestSample(TestSampleType::SolidSphere);
    MCInteractionVolume interactor(sample);
    TS_ASSERT_THROWS(interactor.calculateAbsorption(rng, startPos, direc,
                                                    endPos, lambdaBefore,
                                                    lambdaAfter),
                     std::logic_error);
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  enum class TestSampleType { SolidSphere, Annulus, SamplePlusCan };

  class MockRNG final : public Mantid::Kernel::PseudoRandomNumberGenerator {
  public:
    MOCK_METHOD0(nextValue, double());
    MOCK_METHOD2(nextValue, double(double, double));
    MOCK_METHOD2(nextInt, int(int, int));
    MOCK_METHOD0(restart, void());
    MOCK_METHOD0(save, void());
    MOCK_METHOD0(restore, void());
    MOCK_METHOD1(setSeed, void(size_t));
    MOCK_METHOD2(setRange, void(const double, const double));
  };

  Mantid::API::Sample createTestSample(TestSampleType sampleType) {
    using Mantid::API::Sample;
    using Mantid::Kernel::Material;
    using Mantid::Kernel::V3D;
    using Mantid::Geometry::Object_sptr;
    using Mantid::PhysicalConstants::getNeutronAtom;

    using namespace Mantid::Geometry;

    Sample testSample;
    if (sampleType == TestSampleType::SamplePlusCan) {
      testSample = createSamplePlusCan();
    } else {
      Object_sptr shape;
      if (sampleType == TestSampleType::SolidSphere) {
        shape = ComponentCreationHelper::createSphere(0.1);
      } else if (sampleType == TestSampleType::Annulus) {
        shape = createAnnulus(0.1, 0.15, 0.15, V3D(0, 0, 1));
      } else {
        throw std::invalid_argument("Unknown testing shape type requested");
      }
      shape->setMaterial(Material("Vanadium", getNeutronAtom(23), 0.02));
      testSample.setShape(*shape);
    }
    return testSample;
  }

  Mantid::API::Sample createSamplePlusCan() {
    using Mantid::API::Sample;
    using Mantid::API::SampleEnvironment;
    using Mantid::Kernel::Material;
    using Mantid::Kernel::V3D;
    using Mantid::PhysicalConstants::getNeutronAtom;

    // Create an annulus Vanadium can with silicon sample
    const double height(0.05), innerRadius(0.0046), outerRadius(0.005);
    const V3D centre(0, 0, -0.5 * height), upAxis(0, 0, 1);
    // Can
    auto environment =
        Mantid::Kernel::make_unique<SampleEnvironment>("Annulus Can");
    auto can = createAnnulus(innerRadius, outerRadius, height, upAxis);
    can->setMaterial(Material("Vanadium", getNeutronAtom(23), 0.02));
    environment->add(*can);
    // Sample volume
    auto sampleCell = ComponentCreationHelper::createCappedCylinder(
        innerRadius, height, centre, upAxis, "sample");
    sampleCell->setMaterial(Material("Si", getNeutronAtom(14), 0.15));

    // Sample object
    Sample testSample;
    testSample.setShape(*sampleCell);
    testSample.setEnvironment(environment.release());
    return testSample;
  }

  Mantid::Geometry::Object_sptr
  createAnnulus(double innerRadius, double outerRadius, double height,
                const Mantid::Kernel::V3D &upAxis) const {
    using Mantid::Geometry::ShapeFactory;
    using Mantid::Kernel::V3D;

    // Cylinders oriented along up, with origin at centre of cylinder
    const V3D centre(0, 0, -0.5 * height);
    const std::string inner = ComponentCreationHelper::cappedCylinderXML(
        innerRadius, height, centre, upAxis, "inner");
    const std::string outer = ComponentCreationHelper::cappedCylinderXML(
        outerRadius, height, centre, upAxis, "outer");

    // Combine shapes
    std::ostringstream os;
    os << inner << outer << "<algebra val=\"(outer (# inner))\" />";
    return ShapeFactory().createShape(os.str());
  }
};

#endif /* MANTID_ALGORITHMS_MCINTERACTIONVOLUMETEST_H_ */
