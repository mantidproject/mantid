// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/V3D.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MockRNG.h"

#include <cxxtest/TestSuite.h>
#include <memory>

using Mantid::Geometry::SampleEnvironment;

class SampleEnvironmentTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SampleEnvironmentTest *createSuite() { return new SampleEnvironmentTest(); }
  static void destroySuite(SampleEnvironmentTest *suite) { delete suite; }

  void test_Constructor_Sets_Name_And_Single_Element() {
    using Mantid::Geometry::Container;
    auto can = std::make_shared<Container>("");
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

  void test_The_Container_Turns_With_The_Goniometer_And_Other_Components_Do_Not_By_Default() {
    auto kit = createTestKit();
    TS_ASSERT(kit->isOnGoniometer(0));
    TS_ASSERT(!kit->isOnGoniometer(1));
  }

  void test_Each_Component_Can_Say_Whether_It_Is_On_The_Goniometer() {
    using Mantid::Geometry::Container;
    SampleEnvironment kit("TestKit", std::make_shared<Container>(""), false);
    kit.add(ComponentCreationHelper::createSphere(0.1), true);
    TS_ASSERT(!kit.isOnGoniometer(0));
    TS_ASSERT(kit.isOnGoniometer(1));
  }

  void test_A_Component_Carrying_A_Goniometer_Rotation_Is_Rejected() {
    using namespace Mantid::Geometry;
    auto kit = createTestKit();
    auto baked = ShapeFactory().createShape(ShapeFactory().addGoniometerTag(
        ShapeFactory::generateXRotation(M_PI / 2), ComponentCreationHelper::sphereXML(0.1, {0, 0, 0}, "baked")));

    TS_ASSERT_THROWS(kit->add(baked), const std::invalid_argument &);
    // and through a container, which reports its shape's rotation
    TS_ASSERT_THROWS(SampleEnvironment("TestKit", std::make_shared<Container>(baked)), const std::invalid_argument &);
  }

  void test_In_Lab_Frame_Turns_Mounted_Components_And_Shares_Fixed_Ones() {
    using Mantid::Kernel::V3D;
    const auto kit = createOffsetKit();

    const auto lab = kit->inLabFrame(quarterTurnAboutZ());

    // a quarter turn about z takes +x to +y
    assertCentredOn(lab->getContainer(), V3D(0, 2, 0));
    TS_ASSERT_EQUALS(lab->getComponentPtr(1), kit->getComponentPtr(1));
    assertCentredOn(lab->getComponent(2), V3D(0, 2, 0));
    TS_ASSERT_EQUALS(lab->name(), "OffsetKit");
    TS_ASSERT_EQUALS(lab->containerID(), "can");
    TS_ASSERT(lab->isOnGoniometer(0) && !lab->isOnGoniometer(1) && lab->isOnGoniometer(2));
    // and the original is untouched
    assertCentredOn(kit->getContainer(), V3D(2, 0, 0));
  }

  void test_Turning_A_Lab_Frame_View_Starts_From_The_Original() {
    const auto kit = createOffsetKit();

    const auto twice = kit->inLabFrame(quarterTurnAboutZ())->inLabFrame(quarterTurnAboutZ());

    // on +y, as one turn puts it, not on -x as two would
    assertCentredOn(twice->getContainer(), Mantid::Kernel::V3D(0, 2, 0));
    assertCentredOn(twice->ownFrame().getContainer(), Mantid::Kernel::V3D(2, 0, 0));
  }

  void test_Adding_To_A_Lab_Frame_View_Throws() {
    const auto lab = createOffsetKit()->inLabFrame(quarterTurnAboutZ());
    TS_ASSERT_THROWS(lab->add(ComponentCreationHelper::createSphere(0.1)), const std::logic_error &);
  }

private:
  static Mantid::Kernel::Matrix<double> quarterTurnAboutZ() {
    return Mantid::Kernel::Matrix<double>(std::vector<double>{0, -1, 0, 1, 0, 0, 0, 0, 1});
  }

  static void assertCentredOn(const Mantid::Geometry::IObject &shape, const Mantid::Kernel::V3D &expected) {
    const auto centre = shape.getBoundingBox().centrePoint();
    TS_ASSERT_DELTA(centre.X(), expected.X(), 1e-6);
    TS_ASSERT_DELTA(centre.Y(), expected.Y(), 1e-6);
    TS_ASSERT_DELTA(centre.Z(), expected.Z(), 1e-6);
  }

  /// A can and two components, all on +x so any turn shows: the can and the last are mounted.
  std::shared_ptr<SampleEnvironment> createOffsetKit() {
    using namespace Mantid::Geometry;
    const auto onPlusX = [](const std::string &id) {
      return ShapeFactory().createShape(ComponentCreationHelper::sphereXML(0.5, {2, 0, 0}, id));
    };
    auto can = std::make_shared<Container>(onPlusX("can"));
    can->setID("can");
    auto kit = std::make_shared<SampleEnvironment>("OffsetKit", can);
    kit->add(onPlusX("fixed"));
    kit->add(onPlusX("mounted"), true);
    return kit;
  }

  std::shared_ptr<SampleEnvironment> createTestKit() {
    using namespace Mantid::Geometry;
    using namespace Mantid::Kernel;

    // at centre
    ShapeFactory factory;
    auto can = std::make_shared<Container>(
        factory.createShape(ComponentCreationHelper::sphereXML(0.01, V3D(0, 0, 0), "sp-1")));
    can->setID("8mm");
    auto kit = std::make_shared<SampleEnvironment>("TestKit", can);
    // before sample
    kit->add(ComponentCreationHelper::createSphere(0.1, V3D(-0.25, 0.0, 0.0)));
    // after sample
    kit->add(ComponentCreationHelper::createSphere(0.1, V3D(0.25, 0.0, 0.0)));
    return kit;
  }
};
