// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/SaveSampleEnvironmentAndShape.h"
#include "MantidGeometry/Instrument/Container.h"
#include "MantidGeometry/Instrument/SampleEnvironment.h"
#include "MantidGeometry/Objects/IObject.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/V3D.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

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
  static void
  destroySuite(std::unique_ptr<SaveSampleEnvironmentAndShapeTest> suite) {
    suite.reset(nullptr);
  }

  void testInit() {

    SaveSampleEnvironmentAndShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());
    auto props = alg.getProperties();
    TSM_ASSERT_EQUALS("should be 3 properties here", 3,
                      (size_t)(alg.getProperties().size()));
  }

  void testSimpleShape() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto mesh1 = createCube();
    ws->mutableSample().setShape(mesh1);

    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void testWithEnvironment() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto mesh1 = createCube();
    ws->mutableSample().setShape(mesh1);

    auto mesh2 = createCube();
    mesh2->translate(Kernel::V3D{10, 10, 10});
    auto can = boost::make_shared<Container>(mesh2);
    auto environment = std::make_unique<SampleEnvironment>("name", can);
    ws->mutableSample().setEnvironment(std::move(environment));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void testComplexEnvironment() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto mesh1 = createCube();
    ws->mutableSample().setShape(mesh1);

    auto mesh2 = createCube();
    mesh2->translate(Kernel::V3D{10, 10, 10});
    auto can = boost::make_shared<Container>(mesh2);
    auto environment = std::make_unique<SampleEnvironment>("name", can);
    auto mesh3 = createCube();
    mesh3->translate(Kernel::V3D{-10, -10, -10});
    environment->add(mesh3);
    ws->mutableSample().setEnvironment(std::move(environment));

    TS_ASSERT_THROWS_NOTHING(alg.execute());
  }

  void testFailNoShape() {
    SaveSampleEnvironmentAndShape alg;
    setup(alg);

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  void testFailNotMesh() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const auto container = boost::make_shared<Container>();
    ws->mutableSample().setShape(container);

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  void testFailIncompleteSample() {
    SaveSampleEnvironmentAndShape alg;
    auto ws = setup(alg);

    const std::vector<uint32_t> v1;
    const std::vector<Kernel::V3D> v2;
    const auto mesh =
        boost::make_shared<MeshObject>(v1, v2, Kernel::Material());
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
    const auto mesh2 =
        boost::make_shared<MeshObject>(v1, v2, Kernel::Material());
    auto can = boost::make_shared<Container>(mesh2);
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
    auto can = boost::make_shared<Container>(mesh2);
    auto environment = std::make_unique<SampleEnvironment>("can", can);
    auto mesh3 = boost::make_shared<MeshObject>(v1, v2, Kernel::Material());
    environment->add(mesh3);
    ws->mutableSample().setEnvironment(std::move(environment));

    TS_ASSERT_THROWS(alg.execute(), std::invalid_argument &);
  }

  // setup algorithm properties
  MatrixWorkspace_sptr setup(SaveSampleEnvironmentAndShape &alg) {

    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors,
                                                                     nbins);
    alg.initialize();
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("Filename", "TempFile");
    return inputWS;
  }

  // create a cube mesh object
  boost::shared_ptr<MeshObject> createCube() {
    const std::vector<uint32_t> faces{0, 1, 2, 0, 3, 1, 0, 2, 4, 2, 1, 5,
                                      2, 5, 4, 6, 1, 3, 6, 5, 1, 4, 5, 6,
                                      7, 3, 0, 0, 4, 7, 7, 6, 3, 4, 6, 7};
    const std::vector<Mantid::Kernel::V3D> vertices{
        Mantid::Kernel::V3D(-5, -5, -15), Mantid::Kernel::V3D(5, 5, -15),
        Mantid::Kernel::V3D(5, -5, -15),  Mantid::Kernel::V3D(-5, 5, -15),
        Mantid::Kernel::V3D(5, -5, 15),   Mantid::Kernel::V3D(5, 5, 15),
        Mantid::Kernel::V3D(-5, 5, 15),   Mantid::Kernel::V3D(-5, -5, 15)};
    auto cube = boost::make_shared<MeshObject>(faces, vertices,
                                               Mantid::Kernel::Material());
    return cube;
  }
};