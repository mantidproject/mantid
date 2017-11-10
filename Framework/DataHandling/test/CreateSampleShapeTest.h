#ifndef CREATESAMPLESHAPETEST_H_
#define CREATESAMPLESHAPETEST_H_

//---------------------------------------
// Includes
//---------------------------------------
#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/CreateSampleShape.h"
#include "MantidAPI/MatrixWorkspace.h"
#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/Sample.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidKernel/Material.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

using namespace Mantid::DataHandling;

class CreateSampleShapeTest : public CxxTest::TestSuite {

public:
  void testSphere() {
    using Mantid::Kernel::V3D;
    std::string sphere =
        ComponentCreationHelper::sphereXML(1, V3D(), "some-sphere");

    /// Test passes point inside sphere
    runStandardTest(sphere, 0.5, 0.5, 0.5, true);
    /// Test fails for point outside sphere
    runStandardTest(sphere, 5, 5, 5, false);
  }

  void testCompositeObject() {
    // This is a ball with a cylinder carved out of the middle
    std::string xmldef =
        "<cylinder id=\"stick\">"
        "<centre-of-bottom-base x=\"-0.5\" y=\"0.0\" z=\"0.0\" />"
        "<axis x=\"1.0\" y=\"0.0\" z=\"0.0\" />"
        "<radius val=\"0.05\" />"
        "<height val=\"1.0\" />"
        "</cylinder>"
        "<sphere id=\"some-sphere\">"
        "<centre x=\"0.0\"  y=\"0.0\" z=\"0.0\" />"
        "<radius val=\"0.5\" />"
        "</sphere>"
        "<algebra val=\"some-sphere (# stick)\" />";

    // Inside object
    runStandardTest(xmldef, 0.0, 0.25, 0.25, true);
    // Outside object
    runStandardTest(xmldef, 0.0, 0.0, 0.0, false);
  }

  void test_Setting_Geometry_With_Material_Already_Set_Keeps_Material() {
    using Mantid::Kernel::Material;
    using Mantid::Kernel::V3D;
    using Mantid::PhysicalConstants::getNeutronAtom;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto sampleShape = ComponentCreationHelper::createSphere(0.5);
    sampleShape->setID("mysample");
    Material alum("Al", getNeutronAtom(13), 2.6989);
    sampleShape->setMaterial(alum);
    inputWS->mutableSample().setShape(*sampleShape);

    CreateSampleShape alg;
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue(
        "ShapeXML", ComponentCreationHelper::sphereXML(1.0, V3D(), "sp-1"));
    alg.execute();

    // Old material
    const auto &material = inputWS->sample().getMaterial();
    TS_ASSERT_EQUALS("Al", material.name());
    TS_ASSERT_DELTA(2.6989, material.numberDensity(), 1e-04);
  }

  void runStandardTest(std::string xmlShape, double x, double y, double z,
                       bool inside) {
    // Need a test workspace
    Mantid::API::AnalysisDataService::Instance().add(
        "TestWorkspace",
        WorkspaceCreationHelper::create2DWorkspace123(22, 10, 1));

    CreateSampleShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    alg.setPropertyValue("InputWorkspace", "TestWorkspace");
    alg.setPropertyValue("shapeXML", xmlShape);

    TS_ASSERT_THROWS_NOTHING(alg.execute());

    // Get the created object
    Mantid::API::MatrixWorkspace_sptr ws =
        boost::dynamic_pointer_cast<Mantid::API::MatrixWorkspace>(
            Mantid::API::AnalysisDataService::Instance().retrieve(
                "TestWorkspace"));

    const Mantid::Geometry::Object &sample = ws->sample().getShape();
    Mantid::Kernel::V3D point(x, y, z);

    if (inside) {
      TS_ASSERT_EQUALS(sample.isValid(point), true);
    } else {
      TS_ASSERT_EQUALS(sample.isValid(point), false);
    }

    TS_ASSERT_THROWS_NOTHING(
        Mantid::API::AnalysisDataService::Instance().remove("TestWorkspace"));
  }
};

#endif
