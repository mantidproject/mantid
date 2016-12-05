#ifndef MANTID_DATAHANDLING_SETSAMPLETEST_H_
#define MANTID_DATAHANDLING_SETSAMPLETEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidDataHandling/SetSample.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/Object.h"
#include "MantidGeometry/Objects/Rules.h"
#include "MantidGeometry/Surfaces/Sphere.h"
#include "MantidKernel/ArrayProperty.h"
#include "MantidKernel/ConfigService.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/PropertyManager.h"
#include "MantidAPI/Sample.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include "Poco/File.h"
#include "Poco/Path.h"

#include <fstream>

using Mantid::DataHandling::SetSample;

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
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);
    auto sampleShape = ComponentCreationHelper::createSphere(0.5);
    sampleShape->setID("mysample");
    inputWS->mutableSample().setShape(*sampleShape);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
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

    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);
    auto sampleShape = ComponentCreationHelper::createSphere(0.5);
    sampleShape->setID("mysample");
    Material alum("Al", getNeutronAtom(13), 2.6989);
    sampleShape->setMaterial(alum);
    inputWS->mutableSample().setShape(*sampleShape);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("Geometry", createGenericGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    TS_ASSERT_DELTA(0.02, getSphereRadius(inputWS->sample().getShape()), 1e-08);
    // Old material
    const auto &material = inputWS->sample().getMaterial();
    TS_ASSERT_EQUALS("Al", material.name());
    TS_ASSERT_DELTA(2.6989, material.numberDensity(), 1e-04);
  }

  void test_Setting_Environment_No_Geometry_Overrides() {
    using Mantid::Kernel::ConfigService;
    using Mantid::Geometry::SampleEnvironment;

    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    // Algorithm uses instrument directories as a search location, alter this
    // for the test
    auto &config = ConfigService::Instance();
    const auto defaultDirs = config.getString("instrumentDefinition.directory");
    config.setString("instrumentDefinition.directory", m_testRoot);
    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
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
    using Mantid::Kernel::ConfigService;
    using Mantid::Geometry::SampleEnvironment;

    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);
    auto testInst = ComponentCreationHelper::createTestInstrumentCylindrical(1);
    testInst->setName(m_instName);
    inputWS->setInstrument(testInst);

    // Algorithm uses instrument directories as a search location, alter this
    // for the test
    auto &config = ConfigService::Instance();
    const auto defaultDirs = config.getString("instrumentDefinition.directory");
    config.setString("instrumentDefinition.directory", m_testRoot);
    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
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
    TS_ASSERT_DELTA(0.4, getSphereRadius(sampleShape), 1e-08);
  }

  void test_Setting_Geometry_As_FlatPlate() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("Geometry", createFlatPlateGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    auto tag = sampleShape.getShapeXML().find("cuboid");
    TS_ASSERT(tag != std::string::npos);

    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0, 0.01)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0, 0.0)));
  }

  void test_Setting_Geometry_As_Cylinder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("Geometry", createCylinderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    auto tag = sampleShape.getShapeXML().find("cylinder");
    TS_ASSERT(tag != std::string::npos);

    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.009, 0.015)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, -0.009, 0.015)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.011, 0.015)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, -0.011, 0.015)));
  }

  void test_Setting_Geometry_As_HollowCylinder() {
    using Mantid::Kernel::V3D;
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);
    alg->setProperty("Geometry", createHollowCylinderGeometryProps());
    TS_ASSERT_THROWS_NOTHING(alg->execute());
    TS_ASSERT(alg->isExecuted());

    // New shape
    const auto &sampleShape = inputWS->sample().getShape();
    TS_ASSERT(sampleShape.hasValidShape());
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, 0.009, 0.045)));
    TS_ASSERT_EQUALS(true, sampleShape.isValid(V3D(0, -0.009, 0.045)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, 0.011, 0.045)));
    TS_ASSERT_EQUALS(false, sampleShape.isValid(V3D(0, -0.011, 0.045)));
  }

  //----------------------------------------------------------------------------
  // Failure tests
  //----------------------------------------------------------------------------

  void test_Environment_Args_Without_Name_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);

    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Container", "8mm"), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_Environment_Args_Without_Container_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);

    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Name", m_envName), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  void test_Environment_Args_With_Empty_Strings_Invalid() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;
    auto inputWS = WorkspaceCreationHelper::Create2DWorkspaceBinned(1, 1);

    auto alg = createAlgorithm();
    alg->setProperty("InputWorkspace", inputWS);

    auto args = boost::make_shared<PropertyManager>();
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Name", ""), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
    args->removeProperty("Name");
    args->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Container", ""), "");
    alg->setProperty("Environment", args);
    TS_ASSERT_THROWS(alg->execute(), std::runtime_error);
  }

  //----------------------------------------------------------------------------
  // Non-test methods
  //----------------------------------------------------------------------------
private:
  Mantid::API::IAlgorithm_uptr createAlgorithm() {
    auto alg = Mantid::Kernel::make_unique<SetSample>();
    alg->setChild(true);
    alg->setRethrows(true);
    alg->initialize();
    return std::move(alg);
  }

  Mantid::Kernel::PropertyManager_sptr createMaterialProps() {
    using Mantid::Kernel::PropertyManager;
    using StringProperty = Mantid::Kernel::PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("ChemicalFormula", "V"),
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

  Mantid::Kernel::PropertyManager_sptr createFlatPlateGeometryProps() {
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
    std::vector<double> center{0, 0, 1};
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleArrayProperty>("Center", center), "");

    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createCylinderGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using IntProperty = PropertyWithValue<int64_t>;
    using StringProperty = PropertyWithValue<std::string>;

    auto props = boost::make_shared<PropertyManager>();
    props->declareProperty(
        Mantid::Kernel::make_unique<StringProperty>("Shape", "Cylinder"), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Height", 2), "");
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleProperty>("Radius", 5), "");
    std::vector<double> center{0, 0, 1};
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleArrayProperty>("Center", center), "");
    props->declareProperty(Mantid::Kernel::make_unique<IntProperty>("Axis", 1),
                           "");

    return props;
  }

  Mantid::Kernel::PropertyManager_sptr createHollowCylinderGeometryProps() {
    using namespace Mantid::Kernel;
    using DoubleArrayProperty = ArrayProperty<double>;
    using DoubleProperty = PropertyWithValue<double>;
    using IntProperty = PropertyWithValue<int64_t>;
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
    props->declareProperty(
        Mantid::Kernel::make_unique<DoubleArrayProperty>("Center", center), "");
    props->declareProperty(Mantid::Kernel::make_unique<IntProperty>("Axis", 1),
                           "");

    return props;
  }

  double getSphereRadius(const Mantid::Geometry::Object &shape) {
    using Mantid::Geometry::SurfPoint;
    using Mantid::Geometry::Sphere;
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
