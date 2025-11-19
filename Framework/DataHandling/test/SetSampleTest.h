// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidDataObjects/TableWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PropertyManager.h"

#include <filesystem>
#include <fstream>

using Mantid::DataHandling::SetSample;
using Mantid::Kernel::PropertyWithValue;

class SetSampleTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static SetSampleTest *createSuite() { return new SetSampleTest(); }
  static void destroySuite(SetSampleTest *suite) { delete suite; }

  SetSampleTest() {
    // Setup a temporary directory structure for testing
    std::filesystem::path testDirec(std::filesystem::path::temp(), "SetSampleTest");
    m_testRoot = testDirec.string();
    testDirec.append("sampleenvironments");
    testDirec.makeDirectory();
    testDirec.append(m_facilityName).append(m_instName);
    std::filesystem::create_directories(testDirec);

    // Write test files
    const std::string xml = "<environmentspec>"
                            " <materials>"
                            "  <material id=\"van\" formula=\"V\"/>"
                            " </materials>"
                            " <components>"
                            "  <containers>"
                            "   <container id=\"10mm\" material=\"van\">"
                            "    <geometry>"
                            "     <sphere id=\"sp-1\">"
                            "      <radius val=\"0.1\"/>"
                            "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                            "     </sphere>"
                            "    </geometry>"
                            "    <samplegeometry>"
                            "     <sphere id=\"sp-1\">"
                            "      <radius val=\"0.1\"/>"
                            "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                            "     </sphere>"
                            "    </samplegeometry>"
                            "   </container>"
                            "   <container id =\"10mm_empty\" material=\"van\">"
                            "    <geometry>"
                            "     <sphere id=\"sp-1\">"
                            "      <radius val=\"0.1\"/>"
                            "      <centre x=\"0.0\"  y=\"0.0\" z=\"0.0\"/>"
                            "     </sphere>"
                            "    </geometry>"
                            "   </container>"
                            "  </containers>"
                            " </components>"
                            "</environmentspec>";
    std::filesystem::path envFile(std::filesystem::path(testDirec, m_envName + ".xml"));
    std::ofstream goodStream(envFile.path(), std::ios_base::out);
    goodStream << xml;
    goodStream.close();

    const std::string xml_fixed = "<environmentspec>"
                                  " <materials>"
                                  "  <material id=\"van\" formula=\"V\"/>"
                                  " </materials>"
                                  " <components>"
                                  "  <containers>"
                                  "   <container id=\"10mm\" material=\"van\">"
                                  "    <geometry>"
                                  "     <stlfile filename = \"Sphere10units.stl\" scale =\"mm\" >"
                                  "     </stlfile>"
                                  "    </geometry>"
                                  "    <samplestlfile filename =\"Sphere10units.stl\" scale =\"mm\" >"
                                  "    </samplestlfile>"
                                  "   </container>"
                                  "  </containers>"
                                  " </components>"
                                  "</environmentspec>";
    std::filesystem::path envFileFixed(std::filesystem::path(testDirec, m_envName + "_fixedgeometry.xml"));
    std::ofstream goodStreamFixed(envFileFixed.path(), std::ios_base::out);
    goodStreamFixed << xml_fixed;
    goodStreamFixed.close();
  }

  ~SetSampleTest() {
    try {
      std::filesystem::path(m_testRoot).remove(true);
    } catch (...) {
    }
  }

  //----------------------------------------------------------------------------
  // Success methods
  //----------------------------------------------------------------------------
  void test_Init() {
    SetSample alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  void test_Setting_Material_Alone_Only_Overwrites_Material() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto sampleShape = ComponentCreationHelper::createSphere(0.5);
    sampleShape->setID("mysample");
    inputWS->mutableSample().setShape(sampleShape);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Material", createMaterialProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sampleShapeAfter = inputWS->sample().getShape();
    TS_ASSERT_EQUALS("mysample", sampleShapeAfter.id());
    const auto &material = inputWS->sample().getMaterial();
    TS_ASSERT_EQUALS("V", material.name());
    TS_ASSERT_DELTA(0.0722, material.numberDensity(), 1e-04);
    TS_ASSERT_EQUALS(material.numberDensityEffective(), material.numberDensity())
    TS_ASSERT_EQUALS(material.packingFraction(), 1.0)
  }

  void test_Setting_Container_Material() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto alg = createAlgorithm(inputWS);
    alg->setProperty("ContainerMaterial", createMaterialProps());
    alg->setProperty("ContainerGeometry", createFlatPlateHolderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &container = inputWS->sample().getEnvironment().getContainer();
    const auto &material = container.material();
    TS_ASSERT_EQUALS("V", material.name());
    TS_ASSERT_DELTA(0.0722, material.numberDensity(), 1e-04);
    TS_ASSERT_EQUALS(material.numberDensityEffective(), material.numberDensity())
    TS_ASSERT_EQUALS(material.packingFraction(), 1.0)
  }

  void test_Setting_Geometry_With_Material_Already_Set_Keeps_Material() {
    using Mantid::Kernel::Material;
    using Mantid::PhysicalConstants::getNeutronAtom;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto sampleShape = ComponentCreationHelper::createSphere(0.5);
    Material alum("Al", getNeutronAtom(13), 2.6989);
    sampleShape->setID("mysample");
    sampleShape->setMaterial(alum);
    inputWS->mutableSample().setShape(sampleShape);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createGenericGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    auto &sphere = dynamic_cast<const Mantid::Geometry::CSGObject &>(inputWS->sample().getShape());
    TS_ASSERT_DELTA(0.02, getSphereRadius(sphere), 1e-08);
    // Old material
    const auto &material = inputWS->sample().getMaterial();
    TS_ASSERT_EQUALS("Al", material.name());
    TS_ASSERT_DELTA(2.6989, material.numberDensity(), 1e-04);
    TS_ASSERT_EQUALS(material.numberDensityEffective(), material.numberDensity())
    TS_ASSERT_EQUALS(material.packingFraction(), 1.0)
  }

  void test_Setting_Environment_No_Geometry_Overrides() {
    using Mantid::Geometry::SampleEnvironment;
    using Mantid::Kernel::ConfigService;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    // Algorithm uses instrument directories as a search location, alter this
    // for the test
    auto &config = ConfigService::Instance();
    const auto defaultDirs = config.getString("instrumentDefinition.directory");
    config.setString("instrumentDefinition.directory", m_testRoot);
    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Environment", createEnvironmentProps("10mm"));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    config.setString("instrumentDefinition.directory", defaultDirs);

    // checks
    const auto &sample = inputWS->sample();
    const SampleEnvironment *env(nullptr);
    TS_ASSERT_THROWS_NOTHING(env = &(sample.getEnvironment()));
    TS_ASSERT_EQUALS(m_envName, env->name());
    TS_ASSERT_EQUALS(1, env->nelements());
    TS_ASSERT_EQUALS("10mm", env->containerID());
    const auto &sampleShape = sample.getShape();
    TS_ASSERT(sampleShape.hasValidShape());
  }

  void test_Setting_Environment_With_Geometry_Overrides() {
    using Mantid::Geometry::SampleEnvironment;
    using Mantid::Kernel::ConfigService;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    // Algorithm uses instrument directories as a search location, alter this
    // for the test
    auto &config = ConfigService::Instance();
    const auto defaultDirs = config.getString("instrumentDefinition.directory");
    config.setString("instrumentDefinition.directory", m_testRoot);
    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Environment", createEnvironmentProps("10mm"));
    alg->setProperty("Geometry", createOverrideGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    config.setString("instrumentDefinition.directory", defaultDirs);

    // checks
    const auto &sample = inputWS->sample();
    const SampleEnvironment *env(nullptr);
    TS_ASSERT_THROWS_NOTHING(env = &(sample.getEnvironment()));
    TS_ASSERT_EQUALS(m_envName, env->name());
    TS_ASSERT_EQUALS(1, env->nelements());
    TS_ASSERT_EQUALS("10mm", env->containerID());
    const auto &sampleShape = sample.getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    // New shape
    // radius was 0.1 in <samplegeometry> set in constructor now 0.4
    // from createOverrideGeometryProps
    TS_ASSERT_DELTA(0.4, getSphereRadius(dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape)), 1e-08);
  }

  void test_Setting_Environment_Without_Sample() {
    using Mantid::Geometry::SampleEnvironment;
    using Mantid::Kernel::ConfigService;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    // Algorithm uses instrument directories as a search location, alter this
    // for the test
    auto &config = ConfigService::Instance();
    const auto defaultDirs = config.getString("instrumentDefinition.directory");
    config.setString("instrumentDefinition.directory", m_testRoot);
    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Environment", createEnvironmentProps("10mm_empty"));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    config.setString("instrumentDefinition.directory", defaultDirs);
  }

  void test_Setting_Geometry_As_FlatPlate() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createFlatPlateGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML().find("cuboid");
    TS_ASSERT(tag != std::string::npos);

    // Center
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0.01, 0, 0)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0.0105, 0.025, 0.02)));
    // Origin
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0, 0.0)));
  }

  void test_Setting_Geometry_As_FlatPlate_With_Rotation() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    const double angle(45.0);
    alg->setProperty("Geometry", createFlatPlateGeometryProps(angle));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML().find("cuboid");
    TS_ASSERT(tag != std::string::npos);

    // Center should be preserved inside the shape
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0.01, 0, 0)));
    // V3D(0.0005, 0.025, 0.02) rotated by 45 degrees CCW and translated
    // to center
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(-0.00732412, 0.01803122, 0.02)));
    // End of horizontal axis should now not be inside the object
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.025, 0)));
  }

  void test_Setting_Geometry_As_FlatPlateHolder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setStandardReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createFlatPlateHolderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &sampleShape = sample.getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    const auto xml = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML();
    std::stringstream expectedXML;
    expectedXML << "<type name=\"userShape\">  <cuboid id=\"front\"> "
                   "<left-front-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"-0.009\"/> "
                   "<left-front-top-point x=\"0.004\" y=\"0.0065\" z=\"-0.009\"/> "
                   "<left-back-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"-0.005\"/> "
                   "<right-front-bottom-point x=\"-0.004\" y=\"-0.0065\" "
                   "z=\"-0.009\"/> </cuboid> <cuboid id=\"back\"> "
                   "<left-front-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"0.005\"/> "
                   "<left-front-top-point x=\"0.004\" y=\"0.0065\" z=\"0.005\"/> "
                   "<left-back-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"0.007\"/> "
                   "<right-front-bottom-point x=\"-0.004\" y=\"-0.0065\" z=\"0.005\"/> "
                   "</cuboid><algebra val=\"back:front\"/> </type>";
    TS_ASSERT_EQUALS(xml, expectedXML.str());
  }

  void test_Setting_Container_Geometry_As_FlatPlateHolder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setStandardReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("ContainerGeometry", createFlatPlateHolderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &environment = sample.getEnvironment();
    const auto &can = environment.getContainer();
    const auto &canShape = can.getShape();
    const auto xml = dynamic_cast<const Mantid::Geometry::CSGObject &>(canShape).getShapeXML();
    std::stringstream expectedXML;
    expectedXML << "<type name=\"userShape\">  <cuboid id=\"front\"> "
                   "<left-front-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"-0.009\"/> "
                   "<left-front-top-point x=\"0.004\" y=\"0.0065\" z=\"-0.009\"/> "
                   "<left-back-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"-0.005\"/> "
                   "<right-front-bottom-point x=\"-0.004\" y=\"-0.0065\" "
                   "z=\"-0.009\"/> </cuboid> <cuboid id=\"back\"> "
                   "<left-front-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"0.005\"/> "
                   "<left-front-top-point x=\"0.004\" y=\"0.0065\" z=\"0.005\"/> "
                   "<left-back-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"0.007\"/> "
                   "<right-front-bottom-point x=\"-0.004\" y=\"-0.0065\" z=\"0.005\"/> "
                   "</cuboid><algebra val=\"back:front\"/> </type>";
    TS_ASSERT_EQUALS(xml, expectedXML.str());
  }

  void test_Setting_Geometry_As_FlatPlateHolder_WithCenter() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setStandardReferenceFrame(inputWS);
    const std::vector<double> center = {0., 0., 1.};

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createFlatPlateHolderGeometryProps(0., center));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &sampleShape = sample.getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    const auto xml = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML();
    std::stringstream expectedXML;
    expectedXML << "<type name=\"userShape\">  <cuboid id=\"front\"> "
                   "<left-front-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"0.001\"/> "
                   "<left-front-top-point x=\"0.004\" y=\"0.0065\" z=\"0.001\"/> "
                   "<left-back-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"0.005\"/> "
                   "<right-front-bottom-point x=\"-0.004\" y=\"-0.0065\" "
                   "z=\"0.001\"/> </cuboid> <cuboid id=\"back\"> "
                   "<left-front-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"0.015\"/> "
                   "<left-front-top-point x=\"0.004\" y=\"0.0065\" z=\"0.015\"/> "
                   "<left-back-bottom-point x=\"0.004\" y=\"-0.0065\" z=\"0.017\"/> "
                   "<right-front-bottom-point x=\"-0.004\" y=\"-0.0065\" z=\"0.015\"/> "
                   "</cuboid><algebra val=\"back:front\"/> </type>";
    TS_ASSERT_EQUALS(xml, expectedXML.str());
  }

  void test_Setting_Geometry_As_FlatPlateHolder_WithAngle() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setStandardReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createFlatPlateHolderGeometryProps(90.));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &sampleShape = sample.getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    const auto xml = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML();
    std::stringstream expectedXML;
    expectedXML << "<type name=\"userShape\">  <cuboid id=\"front\"> "
                   "<left-front-bottom-point x=\"-0.009\" y=\"-0.0065\" z=\"-0.004\"/> "
                   "<left-front-top-point x=\"-0.009\" y=\"0.0065\" z=\"-0.004\"/> "
                   "<left-back-bottom-point x=\"-0.005\" y=\"-0.0065\" z=\"-0.004\"/> "
                   "<right-front-bottom-point x=\"-0.009\" y=\"-0.0065\" z=\"0.004\"/> "
                   "</cuboid> <cuboid id=\"back\"> <left-front-bottom-point "
                   "x=\"0.005\" y=\"-0.0065\" z=\"-0.004\"/> <left-front-top-point "
                   "x=\"0.005\" y=\"0.0065\" z=\"-0.004\"/> <left-back-bottom-point "
                   "x=\"0.007\" y=\"-0.0065\" z=\"-0.004\"/> <right-front-bottom-point "
                   "x=\"0.005\" y=\"-0.0065\" z=\"0.004\"/> </cuboid><algebra "
                   "val=\"back:front\"/> </type>";
    TS_ASSERT_EQUALS(xml, expectedXML.str());
  }

  void test_Setting_Geometry_As_HollowCylinderHolder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setStandardReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createHollowCylinderHolderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &sampleShape = sample.getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    const auto xml = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML();
    std::stringstream expectedXML;
    expectedXML << "<type name=\"userShape\"> <hollow-cylinder id=\"inner\"> "
                   "<centre-of-bottom-base x=\"0\" y=\"-0.005\" z=\"0\"/> "
                   "<axis x=\"0\" y=\"1\" z=\"0\"/> <height val=\"0.01\"/> "
                   "<inner-radius val=\"0.001\"/><outer-radius "
                   "val=\"0.002\"/></hollow-cylinder><hollow-cylinder "
                   "id=\"outer\"> <centre-of-bottom-base x=\"0\" y=\"-0.005\" "
                   "z=\"0\"/> <axis x=\"0\" y=\"1\" z=\"0\"/> <height "
                   "val=\"0.01\"/> <inner-radius val=\"0.003\"/><outer-radius "
                   "val=\"0.004\"/></hollow-cylinder><algebra "
                   "val=\"inner:outer\"/> </type>";
    TS_ASSERT_EQUALS(xml, expectedXML.str());
  }

  void test_Setting_Geometry_As_HollowCylinderHolder_WithCenter() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setStandardReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createHollowCylinderHolderGeometryProps({3, 5, 7}));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &sampleShape = sample.getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    const auto xml = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML();
    std::stringstream expectedXML;
    expectedXML << "<type name=\"userShape\"> <hollow-cylinder id=\"inner\"> "
                   "<centre-of-bottom-base x=\"0.03\" y=\"0.045\" z=\"0.07\"/> "
                   "<axis x=\"0\" y=\"1\" z=\"0\"/> <height val=\"0.01\"/> "
                   "<inner-radius val=\"0.001\"/><outer-radius "
                   "val=\"0.002\"/></hollow-cylinder><hollow-cylinder "
                   "id=\"outer\"> <centre-of-bottom-base x=\"0.03\" y=\"0.045\" "
                   "z=\"0.07\"/> <axis x=\"0\" y=\"1\" z=\"0\"/> <height "
                   "val=\"0.01\"/> <inner-radius val=\"0.003\"/><outer-radius "
                   "val=\"0.004\"/></hollow-cylinder><algebra "
                   "val=\"inner:outer\"/> </type>";
    TS_ASSERT_EQUALS(xml, expectedXML.str());
  }

  void SetSample_CSGSphereMergedWithCylinder_DoesNotCrash() {
    // Create a workspace with dummy data
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);

    // Define CSG XML shape (sphere merged with cylinder)
    std::string mergeXML = "<cylinder id=\"stick\">"
                           "<centre-of-bottom-base x=\"-0.5\" y=\"0.0\" z=\"0.0\" />"
                           "<axis x=\"1.0\" y=\"0.0\" z=\"0.0\" />"
                           "<radius val=\"0.05\" />"
                           "<height val=\"1.0\" />"
                           "</cylinder>"
                           "<sphere id=\"some-sphere\">"
                           "<centre x=\"0.7\" y=\"0.0\" z=\"0.0\" />"
                           "<radius val=\"0.2\" />"
                           "</sphere>"
                           "<rotate-all x=\"90\" y=\"-45\" z=\"0\" />"
                           "<algebra val=\"some-sphere (: stick)\" />";

    // Set the sample shape
    auto alg = createAlgorithm(inputWS);
    alg->setProperty("GeometryShape", "CSG");
    alg->setProperty("GeometryValue", mergeXML);
    alg->execute();

    // Check sample shape was set
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &sampleShape = sample.getShape();
    TS_ASSERT(sampleShape.hasValidShape());
  }

  void test_Setting_Container_Geometry_As_HollowCylinderHolder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setStandardReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("ContainerGeometry", createHollowCylinderHolderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &environment = sample.getEnvironment();
    const auto &can = environment.getContainer();
    const auto &canShape = can.getShape();
    const auto xml = dynamic_cast<const Mantid::Geometry::CSGObject &>(canShape).getShapeXML();
    std::stringstream expectedXML;
    expectedXML << "<type name=\"userShape\"> <hollow-cylinder id=\"inner\"> "
                   "<centre-of-bottom-base x=\"0\" y=\"-0.005\" z=\"0\"/> "
                   "<axis x=\"0\" y=\"1\" z=\"0\"/> <height val=\"0.01\"/> "
                   "<inner-radius val=\"0.001\"/><outer-radius "
                   "val=\"0.002\"/></hollow-cylinder><hollow-cylinder "
                   "id=\"outer\"> <centre-of-bottom-base x=\"0\" y=\"-0.005\" "
                   "z=\"0\"/> <axis x=\"0\" y=\"1\" z=\"0\"/> <height "
                   "val=\"0.01\"/> <inner-radius val=\"0.003\"/><outer-radius "
                   "val=\"0.004\"/></hollow-cylinder><algebra "
                   "val=\"inner:outer\"/> </type>";
    TS_ASSERT_EQUALS(xml, expectedXML.str());
  }

  void test_Setting_Container_Geometry_As_HollowCylinder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setStandardReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("ContainerGeometry", createHollowCylinderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    const auto &sample = inputWS->sample();
    const auto &environment = sample.getEnvironment();
    const auto &can = environment.getContainer();
    const auto &canShape = can.getShape();
    const auto xml = dynamic_cast<const Mantid::Geometry::CSGObject &>(canShape).getShapeXML();
    std::stringstream expectedXML;
    expectedXML << "<type name=\"userShape\"> <hollow-cylinder id=\"sample-shape\"> "
                   "<centre-of-bottom-base x=\"0\" y=\"-0.01\" z=\"0.01\"/> <axis "
                   "x=\"0\" y=\"1\" z=\"0\"/><height val=\"0.02\"/> <inner-radius "
                   "val=\"0.03\"/><outer-radius val=\"0.04\"/></hollow-cylinder> "
                   "</type>";
    TS_ASSERT_EQUALS(xml, expectedXML.str());
  }

  void test_Setting_Geometry_As_Cylinder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createCylinderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML().find("cylinder");
    TS_ASSERT(tag != std::string::npos);

    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.019)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.001)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, 0.021)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, -0.001)));
  }

  void test_Setting_Geometry_As_Cylinder_With_Axis() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createCylinderWithAxisGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML().find("cylinder");
    TS_ASSERT(tag != std::string::npos);

    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.019)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.001)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, 0.021)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, -0.001)));
  }

  void test_Setting_Geometry_As_Cylinder_With_Indexed_Axis() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createCylinderWithIndexedAxisGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML().find("cylinder");
    TS_ASSERT(tag != std::string::npos);

    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.019)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.001)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, 0.021)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, -0.001)));
  }

  void test_Setting_Geometry_No_Volume() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);
    // this must match geometry created in createCylinderGeometryProps()
    constexpr double VOLUME = M_PI * 5. * 5. * 2.; // pi * (r^2) * h

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createCylinderGeometryProps());
    alg->setProperty("Material", createMaterialProps(VOLUME));
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape).getShapeXML().find("cylinder");
    TS_ASSERT(tag != std::string::npos);

    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.019)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.001)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, 0.021)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, -0.001)));

    const auto &material = inputWS->sample().getMaterial();
    TS_ASSERT_EQUALS("V", material.name());
    TS_ASSERT_DELTA(0.0722, material.numberDensity(), 1e-04);
    TS_ASSERT_EQUALS(material.numberDensityEffective(), material.numberDensity())
    TS_ASSERT_EQUALS(material.packingFraction(), 1.0)
  }

  void test_Setting_Geometry_As_HollowCylinder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createHollowCylinderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.035, 0.019)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.035, 0.001)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.041, 0.021)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.041, -0.001)));
  }

  void test_Setting_Geometry_As_HollowCylinder_With_Axis() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createHollowCylinderWithAxisGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.035, 0.019)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.035, 0.001)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.041, 0.021)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.041, -0.001)));
  }

  void test_Setting_Geometry_As_HollowCylinder_With_Indexed_Axis() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createHollowCylinderWithIndexedAxisGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.035, 0.019)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.035, 0.001)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.041, 0.021)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.041, -0.001)));
  }

  void test_PeaksWorkspace_Is_Accepted_Workspace_Type() {
    auto inputWS = WorkspaceCreationHelper::createPeaksWorkspace(1);
    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createHollowCylinderWithIndexedAxisGeometryProps());
  }

  void test_flat_plate_holder() {}

  void test_Explicit_Blanks_Accepted_For_Dictionary_Parameters() {
    // when run from algorithm dialog in UI with some dictionary parameters
    // blank
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto sampleShape = ComponentCreationHelper::createSphere(0.5);
    sampleShape->setID("mysample");
    inputWS->mutableSample().setShape(sampleShape);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", "");
    alg->setProperty("Material", createMaterialProps());
    alg->setProperty("Environment", "");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_Setting_center_as_doubles() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto alg = createAlgorithm(inputWS);

    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "Cylinder"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 2), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Radius", 3), "");
    std::vector<double> center{0.0, 0.5, 1.23};
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Center", center), "");

    alg->setProperty("Geometry", props);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_Setting_center_as_longs() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto alg = createAlgorithm(inputWS);

    using namespace Mantid::Kernel;
    using LongArrayProperty = ArrayProperty<long>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "Cylinder"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 2), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Radius", 3), "");
    std::vector<long> center{0, 3, 1};
    props->declareProperty(std::make_unique<LongArrayProperty>("Center", center), "");

    alg->setProperty("Geometry", props);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_Setting_center_as_ints() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto alg = createAlgorithm(inputWS);

    using namespace Mantid::Kernel;
    using IntArrayProperty = ArrayProperty<int>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "Cylinder"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 2), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Radius", 3), "");
    std::vector<int> center{0, 3, 1};
    props->declareProperty(std::make_unique<IntArrayProperty>("Center", center), "");

    alg->setProperty("Geometry", props);
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
  }

  void test_material_properties_correctly_set() {
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto sampleShape = ComponentCreationHelper::createSphere(0.5);
    sampleShape->setID("mysample");
    inputWS->mutableSample().setShape(sampleShape);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", "");
    auto props = createMaterialProps();
    props->declareProperty(std::make_unique<PropertyWithValue<double>>("CoherentXSection", 10.0), "");
    props->declareProperty(std::make_unique<PropertyWithValue<double>>("IncoherentXSection", 5.0), "");
    props->declareProperty(std::make_unique<PropertyWithValue<double>>("AttenuationXSection", 3.0), "");
    props->declareProperty(std::make_unique<PropertyWithValue<double>>("NumberDensity", 2.0), "");
    props->declareProperty(std::make_unique<PropertyWithValue<double>>("EffectiveNumberDensity", 1.25), "");
    alg->setProperty("Material", props);
    alg->setProperty("Environment", "");
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());
    const auto &material = inputWS->sample().getMaterial();
    TS_ASSERT_EQUALS("V", material.name());
    TS_ASSERT_EQUALS(10.0, material.cohScatterXSection());
    TS_ASSERT_EQUALS(5.0, material.incohScatterXSection());
    TS_ASSERT_EQUALS(3.0, material.absorbXSection());
    TS_ASSERT_DELTA(2.0, material.numberDensity(), 1e-04);
    TS_ASSERT_DELTA(1.25, material.numberDensityEffective(), 1e-04);
  }

  void test_run_Geometry_As_Sphere() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    setTestReferenceFrame(inputWS);

    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Geometry", createSphereGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());

    // Check some random points inside sphere
    // Check boundary
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0.049, 0., 0.)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0., 0.049, 0.)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0., 0., 0.049)));
    // Check outside boundary
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0., 0., 0.06)));
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------

  void test_validateArgs_Gives_Errors_For_Incorrect_WorkspaceType() {
    using Mantid::DataObjects::TableWorkspace;
    auto alg = createAlgorithm(std::make_shared<TableWorkspace>(0));

    auto helpMessages = alg->validateInputs();
    TS_ASSERT(helpMessages.find("InputWorkspace") != helpMessages.cend());
  }

  void test_Environment_Args_Without_Name_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm(inputWS);

    auto args = std::make_shared<PropertyManager>();
    args->declareProperty(std::make_unique<StringProperty>("Container", "8mm"), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_Environment_Args_Without_Container_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    auto alg = createAlgorithm(inputWS);

    auto args = std::make_shared<PropertyManager>();
    args->declareProperty(std::make_unique<StringProperty>("Name", m_envName), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), const std::invalid_argument &);
  }

  void test_Environment_Args_With_Empty_Strings_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm(inputWS);

    auto args = std::make_shared<PropertyManager>();
    args->declareProperty(std::make_unique<StringProperty>("Name", ""), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    args->removeProperty("Name");
    args->declareProperty(std::make_unique<StringProperty>("Container", ""), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_Negative_FlatPlate_Dimensions_Give_Validation_Errors() {
    using Mantid::API::IAlgorithm;
    using Mantid::Kernel::PropertyManager;
    using DoubleProperty = Mantid::Kernel::PropertyWithValue<double>;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto alg = createAlgorithm();
    auto args = std::make_shared<PropertyManager>();
    args->declareProperty(std::make_unique<StringProperty>("Shape", "FlatPlate"), "");
    std::array<const std::string, 3> dimensions = {{"Width", "Height", "Thick"}};
    const std::string geometryProp("Geometry");
    for (const auto &dim : dimensions) {
      args->declareProperty(std::make_unique<DoubleProperty>(dim, -1.0), "");
      alg->setProperty(geometryProp, args);
      TS_ASSERT(validateErrorProduced(*alg, geometryProp));
      args->removeProperty(dim);
    }
  }

  void test_Negative_Cylinder_Dimensions_Give_Validation_Errors() {
    using Mantid::Kernel::PropertyManager;
    using DoubleProperty = Mantid::Kernel::PropertyWithValue<double>;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto alg = createAlgorithm();
    auto args = std::make_shared<PropertyManager>();
    args->declareProperty(std::make_unique<StringProperty>("Shape", "Cylinder"), "");
    std::array<const std::string, 2> dimensions = {{"Radius", "Height"}};
    const std::string geometryProp("Geometry");
    for (const auto &dim : dimensions) {
      args->declareProperty(std::make_unique<DoubleProperty>(dim, -1.0), "");
      alg->setProperty(geometryProp, args);
      TS_ASSERT(validateErrorProduced(*alg, geometryProp));
      args->removeProperty(dim);
    }
  }

  void test_Negative_HollowCylinder_Dimensions_Give_Validation_Errors() {
    using Mantid::API::IAlgorithm;
    using Mantid::Kernel::PropertyManager;
    using DoubleProperty = Mantid::Kernel::PropertyWithValue<double>;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto alg = createAlgorithm();
    auto args = std::make_shared<PropertyManager>();
    args->declareProperty(std::make_unique<StringProperty>("Shape", "FlatPlate"), "");
    std::array<const std::string, 3> dimensions = {{"InnerRadius", "OuterRadius", "Height"}};
    const std::string geometryProp("Geometry");
    for (const auto &dim : dimensions) {
      args->declareProperty(std::make_unique<DoubleProperty>(dim, -1.0), "");
      alg->setProperty(geometryProp, args);
      TS_ASSERT(validateErrorProduced(*alg, geometryProp));
      args->removeProperty(dim);
    }
  }

  void test_Geometry_Override_On_Fixed_Sample_Shape_Gives_Error() {
    using Mantid::Geometry::SampleEnvironment;
    using Mantid::Kernel::ConfigService;
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    // Algorithm uses instrument directories as a search location, alter this
    // for the test
    auto &config = ConfigService::Instance();
    const auto defaultDirs = config.getString("instrumentDefinition.directory");
    config.setString("instrumentDefinition.directory", m_testRoot);
    auto alg = createAlgorithm(inputWS);
    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Name", m_envName + "_fixedgeometry"), "");
    alg->setProperty("Environment", props);
    alg->setProperty("Geometry", createOverrideGeometryProps());
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    config.setString("instrumentDefinition.directory", defaultDirs);
  }

  void test_Geometry_Override_On_Environment_Without_Sample_Gives_Error() {
    using Mantid::Geometry::SampleEnvironment;
    using Mantid::Kernel::ConfigService;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    // Algorithm uses instrument directories as a search location, alter this
    // for the test
    auto &config = ConfigService::Instance();
    const auto defaultDirs = config.getString("instrumentDefinition.directory");
    config.setString("instrumentDefinition.directory", m_testRoot);
    auto alg = createAlgorithm(inputWS);
    alg->setProperty("Environment", createEnvironmentProps("10mm_empty"));
    alg->setProperty("Geometry", createOverrideGeometryProps());
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    config.setString("instrumentDefinition.directory", defaultDirs);
  }

  void test_All_Dictionaries_Empty_Gives_Error() {
    using Mantid::Geometry::SampleEnvironment;
    using Mantid::Kernel::ConfigService;

    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    auto alg = createAlgorithm(inputWS);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  Mantid::API::IAlgorithm_uptr
  createAlgorithm(const Mantid::API::Workspace_sptr &inputWS = Mantid::API::Workspace_sptr()) {
    auto alg = std::make_unique<SetSample>();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    if (inputWS) {
      alg->setProperty("InputWorkspace", inputWS);
    }
    return alg;
  }

  bool validateErrorProduced(Mantid::API::IAlgorithm &alg, const std::string &name) {
    const auto errors = alg.validateInputs();
    if (errors.find(name) != errors.end())
      return true;
    else
      return false;
  }

  void setTestReferenceFrame(const Mantid::API::MatrixWorkspace_sptr &workspace) {
    using Mantid::Geometry::Instrument;
    using Mantid::Geometry::ReferenceFrame;
    // Use Z=up,Y=across,X=beam so we test it listens to the reference frame
    auto inst = std::make_shared<Instrument>();
    inst->setReferenceFrame(
        std::make_shared<ReferenceFrame>(Mantid::Geometry::Z, Mantid::Geometry::X, Mantid::Geometry::Right, ""));
    workspace->setInstrument(inst);
  }

  void setStandardReferenceFrame(const Mantid::API::MatrixWorkspace_sptr &workspace) {
    using Mantid::Geometry::Instrument;
    using Mantid::Geometry::ReferenceFrame;
    auto inst = std::make_shared<Instrument>();
    inst->setReferenceFrame(
        std::make_shared<ReferenceFrame>(Mantid::Geometry::Y, Mantid::Geometry::Z, Mantid::Geometry::Right, ""));
    workspace->setInstrument(inst);
  }

  Mantid::Kernel::PropertyManager_sptr createMaterialProps(const double volume = 0.) {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("ChemicalFormula", "V"), "");
    if (volume > 0.) // <mass> = <standard mass density for vanadium> x <volume>
      props->declareProperty(std::make_unique<PropertyWithValue<double>>("SampleMass", 6.11 * volume), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createGenericGeometryProps() {
    using Mantid::Kernel::PropertyManager;
    using Mantid::Kernel::V3D;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "CSG"), "");
    props->declareProperty(
        std::make_unique<StringProperty>("Value", ComponentCreationHelper::sphereXML(0.02, V3D(), "sp-1")), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createEnvironmentProps(const std::string &containerName) {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Name", m_envName), "");
    props->declareProperty(std::make_unique<StringProperty>("Container", containerName), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createOverrideGeometryProps() {
    using Mantid::Kernel::PropertyManager;
    using DoubleProperty = Mantid::Kernel::PropertyWithValue<double>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<DoubleProperty>("Radius", 40), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createFlatPlateGeometryProps(double angle = 0.0) {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "FlatPlate"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Width", 5), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 4), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Thick", 0.1), "");
    std::vector<double> center{1, 0, 0};
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Center", std::move(center)), "");
    if (angle != 0.0) {
      props->declareProperty(std::make_unique<DoubleProperty>("Angle", angle), "");
    }
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createCylinderGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "Cylinder"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 2), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Radius", 5), "");
    std::vector<double> center{0, 0, 1};
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Center", std::move(center)), "");

    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createCylinderWithAxisGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    auto props = createCylinderGeometryProps();
    // Use the same pointing up direction as in the without axis test
    std::vector<double> axis{0, 0, 1};
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Axis", axis), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createCylinderWithIndexedAxisGeometryProps() {
    using namespace Mantid::Kernel;
    using IntProperty = PropertyWithValue<int>;
    auto props = createCylinderGeometryProps();
    // Use the same pointing up direction as in the without axis test
    int axis{2};
    props->declareProperty(std::make_unique<IntProperty>("Axis", axis), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createHollowCylinderGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "HollowCylinder"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 2), "");
    props->declareProperty(std::make_unique<DoubleProperty>("InnerRadius", 3), "");
    props->declareProperty(std::make_unique<DoubleProperty>("OuterRadius", 4), "");
    std::vector<double> center{0, 0, 1};
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Center", std::move(center)), "");

    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createHollowCylinderWithAxisGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    auto props = createHollowCylinderGeometryProps();
    // Use the same pointing up direction as in the without axis test
    std::vector<double> axis{0, 0, 1};
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Axis", axis), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createHollowCylinderWithIndexedAxisGeometryProps() {
    using namespace Mantid::Kernel;
    using IntProperty = PropertyWithValue<int>;
    auto props = createHollowCylinderGeometryProps();
    // Use the same pointing up direction as in the without axis test
    int axis{2};
    props->declareProperty(std::make_unique<IntProperty>("Axis", axis), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createFlatPlateHolderGeometryProps(double angle = 0.,
                                                                          std::vector<double> center = {0., 0., 0.}) {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;
    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "FlatPlateHolder"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 1.3), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Width", 0.8), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Thick", 1.), "");
    props->declareProperty(std::make_unique<DoubleProperty>("FrontThick", 0.4), "");
    props->declareProperty(std::make_unique<DoubleProperty>("BackThick", 0.2), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Angle", angle), "");
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Center", center), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createHollowCylinderHolderGeometryProps(std::vector<double> center = {0., 0.,
                                                                                                             0.},
                                                                               std::vector<double> axis = {0, 1, 0}) {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;
    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "HollowCylinderHolder"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Height", 1.), "");
    props->declareProperty(std::make_unique<DoubleProperty>("InnerRadius", 0.1), "");
    props->declareProperty(std::make_unique<DoubleProperty>("InnerOuterRadius", 0.2), "");
    props->declareProperty(std::make_unique<DoubleProperty>("OuterInnerRadius", 0.3), "");
    props->declareProperty(std::make_unique<DoubleProperty>("OuterRadius", 0.4), "");
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Center", center), "");
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Axis", axis), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createSphereGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = std::make_shared<PropertyManager>();
    props->declareProperty(std::make_unique<StringProperty>("Shape", "Sphere"), "");
    props->declareProperty(std::make_unique<DoubleProperty>("Radius", 5), "");
    std::vector<double> center{0, 0, 1};
    props->declareProperty(std::make_unique<DoubleArrayProperty>("Center", center), "");

    return props;
  }

  double getSphereRadius(const Mantid::Geometry::CSGObject &shape) {
    using Mantid::Geometry::Sphere;
    using Mantid::Geometry::SurfPoint;
    auto topRule = shape.topRule();
    if (auto surfpoint = dynamic_cast<const SurfPoint *>(topRule)) {
      if (auto sphere = dynamic_cast<Sphere *>(surfpoint->getKey())) {
        return sphere->getRadius();
      } else {
        throw std::runtime_error("Expected Sphere as SurfPoint key.");
      }
    } else {
      throw std::runtime_error("Expected SurfPoint as top rule");
    }
  }

  std::string m_testRoot;
  // Use the TEST_LIVE entry in Facilities
  const std::string m_facilityName = "TEST_LIVE";
  const std::string m_instName = "ISIS_Histogram";
  const std::string m_envName = "TestEnv";
};
