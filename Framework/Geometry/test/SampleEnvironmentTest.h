// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef TESTSAMPLEENVIRONMENT_H_
#define TESTSAMPLEENVIRONMENT_H_

#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MockRNG.h"

#include <boost/make_shared.hpp>
#include <cxxtest/TestSuite.h>

using Mantid::Geometry::SampleEnvironment;

class SampleEnvironmentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleEnvironmentTest *createSuite() {
    return new SampleEnvironmentTest();
  }
  static void destroySuite(SampleEnvironmentTest *suite) { delete suite; }

  void test_Constructor_Sets_Name_And_Single_Element() {
    using Mantid::Geometry::Container;
    auto can = boost::make_shared<Container>("");
    can->setID("8mm");

    SampleEnvironment kit("TestKit", can);
    TS_ASSERT_EQUALS(kit.name(), "TestKit");
    TS_ASSERT_EQUALS(kit.containerID(), "8mm");
    TS_ASSERT_EQUALS(1, kit.nelements());
  }

  void test_Adding_Component_Increases_Size_By_One() {
    auto kit = createTestKit();
    TS_ASSERT_EQUALS(3, kit->nelements());
    auto shape = ComponentCreationHelper::createSphere(1.0);
    TS_ASSERT_THROWS_NOTHING(kit->add(shape));
    TS_ASSERT_EQUALS(4, kit->nelements());
    TS_ASSERT_EQUALS(kit->name(), "TestKit");
    TS_ASSERT_EQUALS(kit->containerID(), "8mm");
  }

  void test_IsValid_Tests_All_Components() {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    auto kit = createTestKit();
    // inside can
    TS_ASSERT(kit->isValid(V3D(0, 0, 0)));
    // outside everything
    TS_ASSERT(kit->isValid(V3D(-0.3, 0, 0)));
    // inside object before sample
    TS_ASSERT(kit->isValid(V3D(-0.25, 0, 0)));
    // inside object after sample
    TS_ASSERT(kit->isValid(V3D(0.25, 0, 0)));
  }

  void test_Track_Intersection_Tests_All_Components() {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    auto kit = createTestKit();
    Track ray(V3D(-0.5, 0, 0), V3D(1.0, 0.0, 0.0));
    int nsegments(0);
    TS_ASSERT_THROWS_NOTHING(nsegments = kit->interceptSurfaces(ray));
    TS_ASSERT_EQUALS(3, nsegments);
    TS_ASSERT_EQUALS(3, ray.count());
  }

  void test_BoundingBox_Encompasses_Whole_Object() {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    auto kit = createTestKit();
    auto bbox = kit->boundingBox();

    auto widths = bbox.width();
    TS_ASSERT_DELTA(0.7, widths.X(), 1e-12);
    TS_ASSERT_DELTA(0.2, widths.Y(), 1e-12);
    TS_ASSERT_DELTA(0.2, widths.Z(), 1e-12);
  }

  void testGeneratePointConsidersAllComponents() {
    using namespace ::testing;
    using namespace Mantid::Kernel;

    auto kit = createTestKit();
    size_t maxAttempts(1);

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
    TS_ASSERT_THROWS_NOTHING(comp1Point = kit->generatePoint(rng, maxAttempts));
    TS_ASSERT(kit->isValid(comp1Point));
    Mock::VerifyAndClearExpectations(&rng);

    // Selects second component
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(2));
    EXPECT_CALL(rng, nextValue())
        .Times(3)
        .WillOnce(Return(0.55))
        .WillOnce(Return(0.65))
        .WillOnce(Return(0.70));
    V3D comp2Point;
    TS_ASSERT_THROWS_NOTHING(comp2Point = kit->generatePoint(rng, maxAttempts));
    TS_ASSERT(comp2Point != comp1Point);
    TS_ASSERT(kit->isValid(comp2Point));
    Mock::VerifyAndClearExpectations(&rng);

    // Selects third component
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(3));
    EXPECT_CALL(rng, nextValue())
        .Times(3)
        .WillOnce(Return(0.55))
        .WillOnce(Return(0.65))
        .WillOnce(Return(0.70));
    V3D comp3Point;
    TS_ASSERT_THROWS_NOTHING(comp3Point = kit->generatePoint(rng, maxAttempts));
    TS_ASSERT(comp3Point != comp2Point);
    TS_ASSERT(comp3Point != comp1Point);
    TS_ASSERT(kit->isValid(comp3Point));
    Mock::VerifyAndClearExpectations(&rng);
  }

  void testGeneratePointRespectsActiveRegion() {
    using namespace ::testing;
    using namespace Mantid::Kernel;

    auto kit = createTestKit();
    size_t maxAttempts(1);

    // Generate "random" sequence
    MockRNG rng;
    // Sequence will try to select one of the non-container pieces
    EXPECT_CALL(rng, nextInt(_, _)).Times(Exactly(1)).WillOnce(Return(2));
    EXPECT_CALL(rng, nextValue())
        .Times(3)
        .WillOnce(Return(0.5))
        .WillOnce(Return(0.5))
        .WillOnce(Return(0.5));
    // Restrict region to can
    TS_ASSERT_THROWS(kit->generatePoint(rng,
                                        kit->getContainer().getBoundingBox(),
                                        maxAttempts),
                     const std::runtime_error &);
    Mock::VerifyAndClearExpectations(&rng);
  }

private:
  boost::shared_ptr<SampleEnvironment> createTestKit() {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    // at centre
    ShapeFactory factory;
    auto can = boost::make_shared<Container>(factory.createShape(
        ComponentCreationHelper::sphereXML(0.01, V3D(0, 0, 0), "sp-1")));
    can->setID("8mm");
    auto kit = boost::make_shared<SampleEnvironment>("TestKit", can);
    // before sample
    kit->add(ComponentCreationHelper::createSphere(0.1, V3D(-0.25, 0.0, 0.0)));
    // after sample
    kit->add(ComponentCreationHelper::createSphere(0.1, V3D(0.25, 0.0, 0.0)));
    return kit;
  }
};

#endif // TESTSAMPLEENVIRONMENT_H_
