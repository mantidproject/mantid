// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/SaveSampleEnvironmentAndShape.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/V3D.h"

#include <Poco/File.h>
#include <Poco/Path.h>
#include <cxxtest/TestSuite.h>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::Geometry;

class SaveSampleEnvironmentAndShapeTest : public CxxTest::TestSuite {
public:
  static std::unique_ptr<SaveSampleEnvironmentAndShapeTest> createSuite() {
    return std::make_unique<SaveSampleEnvironmentAndShapeTest>();
  }
  static void destroySuite(std::unique_ptr<SaveSampleEnvironmentAndShapeTest> suite) { suite.reset(nullptr); }

  void testInit() {

    SaveSampleEnvironmentAndShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    auto props = alg.getProperties();
    TSM_ASSERT_EQUALS("should be 3 properties here", 3, (size_t)(alg.getProperties().size()));
  }

  void testSimpleShape() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto mesh1 = createCube();
    ws->mutableSample().setShape(mesh1);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    auto loadMesh = retrieveSavedMesh();
    assertVectorsMatch(*mesh1, *loadMesh);
    Poco::File(m_OutputFile).remove();
  }

  void testWithEnvironment() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto mesh1 = createCube();
    ws->mutableSample().setShape(mesh1);

    auto mesh2 = createCube();
    mesh2->translate(Kernel::V3D{10, 10, 10});
    auto can = std::make_shared<Container>(mesh2);
    auto environment = std::make_unique<SampleEnvironment>("name", can);
    ws->mutableSample().setEnvironment(std::move(environment));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    auto loadMesh = retrieveSavedMesh();
    auto checkMesh = createCubes(2, 10);
    assertVectorsMatch(*loadMesh, *checkMesh);
    Poco::File(m_OutputFile).remove();
  }

  void testComplexEnvironment() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto mesh1 = createCube();
    ws->mutableSample().setShape(mesh1);

    auto mesh2 = createCube();
    mesh2->translate(Kernel::V3D{10, 10, 10});
    auto can = std::make_shared<Container>(mesh2);
    auto environment = std::make_unique<SampleEnvironment>("name", can);
    auto mesh3 = createCube();
    mesh3->translate(Kernel::V3D{20, 20, 20});
    environment->add(mesh3);
    ws->mutableSample().setEnvironment(std::move(environment));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
    auto loadMesh = retrieveSavedMesh();
    auto checkMesh = createCubes(3, 10);
    assertVectorsMatch(*loadMesh, *checkMesh);
    Poco::File(m_OutputFile).remove();
  }

  void testFailNoShape() {
    SaveSampleEnvironmentAndShape alg;
    setup(alg);

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  void testFailNotMesh() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto container = std::make_shared<Container>();
    ws->mutableSample().setShape(container);

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  void testFailIncompleteSample() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const std::vector<uint32_t> v1;
    const std::vector<Kernel::V3D> v2;
    const auto mesh = std::make_shared<MeshObject>(v1, v2, Kernel::Material());
    ws->mutableSample().setShape(mesh);

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  void testFailIncompleteEnvironmentCan() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto mesh1 = createCube();
    ws->mutableSample().setShape(mesh1);

    const std::vector<uint32_t> v1;
    const std::vector<Kernel::V3D> v2;
    const auto mesh2 = std::make_shared<MeshObject>(v1, v2, Kernel::Material());
    auto can = std::make_shared<Container>(mesh2);
    auto environment = std::make_unique<SampleEnvironment>("name", can);
    ws->mutableSample().setEnvironment(std::move(environment));

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  void testFailIncompleteEnvironmentComponent() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto mesh1 = createCube();
    ws->mutableSample().setShape(mesh1);

    auto mesh2 = createCube();
    const std::vector<uint32_t> v1;
    const std::vector<Kernel::V3D> v2;
    auto can = std::make_shared<Container>(mesh2);
    auto environment = std::make_unique<SampleEnvironment>("can", can);
    auto mesh3 = std::make_shared<MeshObject>(v1, v2, Kernel::Material());
    environment->add(mesh3);
    ws->mutableSample().setEnvironment(std::move(environment));

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  // setup algorithm properties
  MatrixWorkspace_sptr setup(SaveSampleEnvironmentAndShape &alg) {

    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors, nbins);
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("Filename", m_OutputFile);
    return inputWS;
  }

  std::shared_ptr<MeshObject> retrieveSavedMesh() {
    LoadBinaryStl loader = LoadBinaryStl(m_OutputFile, ScaleUnits::metres);
    TS_ASSERT(loader.isBinarySTL(m_OutputFile))
    auto shape = loader.readShape();
    return std::shared_ptr<MeshObject>(shape.release());
  }

  void assertVectorsMatch(const MeshObject &mesh1, const MeshObject &mesh2) {
    auto vertices1 = mesh1.getV3Ds();
    auto vertices2 = mesh2.getV3Ds();
    auto triangles1 = mesh1.getTriangles();
    auto triangles2 = mesh2.getTriangles();
    for (size_t i = 0; i < vertices1.size(); ++i) {
      TS_ASSERT_EQUALS(vertices1[i], vertices2[i]);
    }
    for (size_t i = 0; i < triangles1.size(); ++i) {
      TS_ASSERT_EQUALS(triangles1[i], triangles2[i]);
    }
  }

  // create a cube mesh object
  std::shared_ptr<MeshObject> createCube() {
    const std::vector<uint32_t> faces{0, 1, 2, 0, 3, 1, 0, 2, 4, 2, 1, 5, 2, 5, 4, 6, 1, 3,
                                      6, 5, 1, 4, 5, 6, 7, 3, 0, 0, 4, 7, 7, 6, 3, 4, 6, 7};
    const std::vector<Mantid::Kernel::V3D> vertices{Mantid::Kernel::V3D(-5, -5, -15), Mantid::Kernel::V3D(5, 5, -15),
                                                    Mantid::Kernel::V3D(5, -5, -15),  Mantid::Kernel::V3D(-5, 5, -15),
                                                    Mantid::Kernel::V3D(5, -5, 15),   Mantid::Kernel::V3D(5, 5, 15),
                                                    Mantid::Kernel::V3D(-5, 5, 15),   Mantid::Kernel::V3D(-5, -5, 15)};
    auto cube = std::make_shared<MeshObject>(faces, vertices, Mantid::Kernel::Material());
    return cube;
  }
  // create a mesh of cubes for comparison
  std::shared_ptr<MeshObject> createCubes(int num, int translation) {
    uint32_t offset = 0;
    int actualTranslation = 0;
    std::vector<uint32_t> faces;
    std::vector<Mantid::Kernel::V3D> vertices;
    for (int i = 0; i < num; ++i) {
      faces.insert(std::end(faces),
                   {0 + offset, 1 + offset, 2 + offset, 0 + offset, 3 + offset, 1 + offset, 0 + offset, 2 + offset,
                    4 + offset, 2 + offset, 1 + offset, 5 + offset, 2 + offset, 5 + offset, 4 + offset, 6 + offset,
                    1 + offset, 3 + offset, 6 + offset, 5 + offset, 1 + offset, 4 + offset, 5 + offset, 6 + offset,
                    7 + offset, 3 + offset, 0 + offset, 0 + offset, 4 + offset, 7 + offset, 7 + offset, 6 + offset,
                    3 + offset, 4 + offset, 6 + offset, 7 + offset});
      vertices.insert(std::end(vertices),
                      {Mantid::Kernel::V3D(-5 + actualTranslation, -5 + actualTranslation, -15 + actualTranslation),
                       Mantid::Kernel::V3D(5 + actualTranslation, 5 + actualTranslation, -15 + actualTranslation),
                       Mantid::Kernel::V3D(5 + actualTranslation, -5 + actualTranslation, -15 + actualTranslation),
                       Mantid::Kernel::V3D(-5 + actualTranslation, 5 + actualTranslation, -15 + actualTranslation),
                       Mantid::Kernel::V3D(5 + actualTranslation, -5 + actualTranslation, 15 + actualTranslation),
                       Mantid::Kernel::V3D(5 + actualTranslation, 5 + actualTranslation, 15 + actualTranslation),
                       Mantid::Kernel::V3D(-5 + actualTranslation, 5 + actualTranslation, 15 + actualTranslation),
                       Mantid::Kernel::V3D(-5 + actualTranslation, -5 + actualTranslation, 15 + actualTranslation)});
      actualTranslation += translation;
      offset += 8;
    }
    auto cube = std::make_shared<MeshObject>(faces, vertices, Mantid::Kernel::Material());
    return cube;
  }

  const std::string m_OutputFile = Poco::Path::current() + "SaveSampleTest.stl";
};
