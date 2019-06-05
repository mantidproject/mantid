// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef MANTID_DATAHANDLING_SETSAMPLETEST_H_
#define MANTID_DATAHANDLING_SETSAMPLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidAPI/Sample.h"
#include "MantidDataHandling/SetSample.h"
#include "MantidGeometry/Instrument/ReferenceFrame.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/CSGObject.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "Poco/File.h"
#include "Poco/Path.h"

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
    Poco::Path testDirec(Poco::Path::temp(), "SetSampleTest");
    m_testRoot = testDirec.toString();
    testDirec.append("sampleenvironments");
    testDirec.makeDirectory();
    testDirec.append(m_facilityName).append(m_instName);
    Poco::File(testDirec).createDirectories();

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
                            "  </containers>"
                            " </components>"
                            "</environmentspec>";
    Poco::File envFile(Poco::Path(testDirec, m_envName + ".xml"));
    std::ofstream goodStream(envFile.path(), std::ios_base::out);
    goodStream << xml;
    goodStream.close();
  }

  ~SetSampleTest() {
    try {
      Poco::File(m_testRoot).remove(true);
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
    auto &sphere = dynamic_cast<const Mantid::Geometry::CSGObject &>(
        inputWS->sample().getShape());
    TS_ASSERT_DELTA(0.02, getSphereRadius(sphere), 1e-08);
    // Old material
    const auto &material = inputWS->sample().getMaterial();
    TS_ASSERT_EQUALS("Al", material.name());
    TS_ASSERT_DELTA(2.6989, material.numberDensity(), 1e-04);
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
    alg->setProperty("Environment", createEnvironmentProps());
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
    alg->setProperty("Environment", createEnvironmentProps());
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
    TS_ASSERT_DELTA(
        0.4,
        getSphereRadius(
            dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape)),
        1e-08);
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
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape)
                   .getShapeXML()
                   .find("cuboid");
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
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape)
                   .getShapeXML()
                   .find("cuboid");
    TS_ASSERT(tag != std::string::npos);

    // Center should be preserved inside the shape
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0.01, 0, 0)));
    // V3D(0.0005, 0.025, 0.02) rotated by 45 degrees CCW and translated
    // to center
    TS_ASSERT_EQUALS(true,
                     sampleShape.isValid(V3D(-0.00732412, 0.01803122, 0.02)));
    // End of horizontal axis should now not be inside the object
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.025, 0)));
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
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape)
                   .getShapeXML()
                   .find("cylinder");
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
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape)
                   .getShapeXML()
                   .find("cylinder");
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
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape)
                   .getShapeXML()
                   .find("cylinder");
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
    auto tag = dynamic_cast<const Mantid::Geometry::CSGObject &>(sampleShape)
                   .getShapeXML()
                   .find("cylinder");
    TS_ASSERT(tag != std::string::npos);

    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.019)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.049, 0.001)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, 0.021)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.06, -0.001)));

    const auto &material = inputWS->sample().getMaterial();
    TS_ASSERT_EQUALS("V", material.name());
    TS_ASSERT_DELTA(0.0722, material.numberDensity(), 1e-04);
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
    alg->setProperty("Geometry",
                     createHollowCylinderWithIndexedAxisGeometryProps());
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
  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------

  void test_Environment_Args_Without_Name_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm(inputWS);

    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Container", "8mm"), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_Environment_Args_Without_Container_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm(inputWS);

    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Name", m_envName), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_Environment_Args_With_Empty_Strings_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm(inputWS);

    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Name", ""), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
    args->removeProperty("Name");
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Container", ""), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), const std::runtime_error &);
  }

  void test_Negative_FlatPlate_Dimensions_Give_Validation_Errors() {
    using Mantid::API::IAlgorithm;
    using Mantid::Kernel::PropertyManager;
    using DoubleProperty = Mantid::Kernel::PropertyWithValue<double>;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto alg = createAlgorithm();
    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "FlatPlate"), "");
    std::array<const std::string, 3> dimensions = {
        {"Width", "Height", "Thick"}};
    const std::string geometryProp("Geometry");
    for (const auto &dim : dimensions) {
      args->declareProperty(
          Mantid::Kernel::make_unique<DoubleProperty>(dim, -1.0), "");
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
    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "Cylinder"), "");
    std::array<const std::string, 2> dimensions = {{"Radius", "Height"}};
    const std::string geometryProp("Geometry");
    for (const auto &dim : dimensions) {
      args->declareProperty(
          Mantid::Kernel::make_unique<DoubleProperty>(dim, -1.0), "");
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
    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "FlatPlate"), "");
    std::array<const std::string, 3> dimensions = {
        {"InnerRadius", "OuterRadius", "Height"}};
    const std::string geometryProp("Geometry");
    for (const auto &dim : dimensions) {
      args->declareProperty(
          Mantid::Kernel::make_unique<DoubleProperty>(dim, -1.0), "");
      alg->setProperty(geometryProp, args);
      TS_ASSERT(validateErrorProduced(*alg, geometryProp));
      args->removeProperty(dim);
    }
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  Mantid::API::IAlgorithm_uptr
  createAlgorithm(const Mantid::API::MatrixWorkspace_sptr &inputWS =
                      Mantid::API::MatrixWorkspace_sptr()) {
    auto alg = Mantid::Kernel::make_unique<SetSample>();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    if (inputWS) {
      alg->setProperty("InputWorkspace", inputWS);
    }
    return std::move(alg);
  }

  bool validateErrorProduced(Mantid::API::IAlgorithm &alg,
                             const std::string &name) {
    const auto errors = alg.validateInputs();
    if (errors.find(name) != errors.end())
      return true;
    else
      return false;
  }

  void setTestReferenceFrame(Mantid::API::MatrixWorkspace_sptr workspace) {
    using Mantid::Geometry::Instrument;
    using Mantid::Geometry::ReferenceFrame;
    // Use Z=up,Y=across,X=beam so we test it listens to the reference frame
    auto inst = boost::make_shared<Instrument>();
    inst->setReferenceFrame(boost::make_shared<ReferenceFrame>(
        Mantid::Geometry::Z, Mantid::Geometry::X, Mantid::Geometry::Right, ""));
    workspace->setInstrument(inst);
  }

  Mantid::Kernel::PropertyManager_sptr
  createMaterialProps(const double volume = 0.) {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("ChemicalFormula", "V"),
        "");
    if (volume > 0.) // <mass> = <standard mass density for vanadium> x <volume>
      props->declareProperty(
          Mantid::Kernel::make_unique<PropertyWithValue<double>>("SampleMass",
                                                                 6.11 * volume),
          "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createGenericGeometryProps() {
    using Mantid::Kernel::PropertyManager;
    using Mantid::Kernel::V3D;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "CSG"), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>(
            "Value", ComponentCreationHelper::sphereXML(0.02, V3D(), "sp-1")),
        "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createEnvironmentProps() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Name", m_envName), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Container", "10mm"), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createOverrideGeometryProps() {
    using Mantid::Kernel::PropertyManager;
    using DoubleProperty = Mantid::Kernel::PropertyWithValue<double>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Radius", 40), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr
  createFlatPlateGeometryProps(double angle = 0.0) {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "FlatPlate"), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Width", 5), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Height", 4), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Thick", 0.1), "");
    std::vector<double> center{1, 0, 0};
    props->declareProperty(Mantid::Kernel::make_unique<DoubleArrayProperty>(
                               "Center", std::move(center)),
                           "");
    if (angle != 0.0) {
      props->declareProperty(
          Mantid::Kernel::make_unique<DoubleProperty>("Angle", angle), "");
    }
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createCylinderGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "Cylinder"), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Height", 2), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Radius", 5), "");
    std::vector<double> center{0, 0, 1};
    props->declareProperty(Mantid::Kernel::make_unique<DoubleArrayProperty>(
                               "Center", std::move(center)),
                           "");

    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createCylinderWithAxisGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    auto props = createCylinderGeometryProps();
    // Use the same pointing up direction as in the without axis test
    std::vector<double> axis{0, 0, 1};
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleArrayProperty>("Axis", axis), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr
  createCylinderWithIndexedAxisGeometryProps() {
    using namespace Mantid::Kernel;
    using IntProperty = PropertyWithValue<int>;
    auto props = createCylinderGeometryProps();
    // Use the same pointing up direction as in the without axis test
    int axis = 2;
    props->declareProperty(
        Mantid::Kernel::make_unique<IntProperty>("Axis", axis), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createHollowCylinderGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "HollowCylinder"),
        "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Height", 2), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("InnerRadius", 3), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("OuterRadius", 4), "");
    std::vector<double> center{0, 0, 1};
    props->declareProperty(Mantid::Kernel::make_unique<DoubleArrayProperty>(
                               "Center", std::move(center)),
                           "");

    return props;
  }

  Mantid::Kernel::PropertyManager_sptr
  createHollowCylinderWithAxisGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    auto props = createHollowCylinderGeometryProps();
    // Use the same pointing up direction as in the without axis test
    std::vector<double> axis{0, 0, 1};
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleArrayProperty>("Axis", axis), "");
    return props;
  }

  Mantid::Kernel::PropertyManager_sptr
  createHollowCylinderWithIndexedAxisGeometryProps() {
    using namespace Mantid::Kernel;
    using IntProperty = PropertyWithValue<int>;
    ;
    auto props = createHollowCylinderGeometryProps();
    // Use the same pointing up direction as in the without axis test
    int axis = 2;
    props->declareProperty(
        Mantid::Kernel::make_unique<IntProperty>("Axis", axis), "");
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

#endif /* MANTID_ALGORITHMS_SETSAMPLETEST_H_ */
