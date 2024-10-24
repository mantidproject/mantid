// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidDataObjects/PeaksWorkspace.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/OptionalBool.h"

#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class LoadSampleShapeTest : public CxxTest::TestSuite {
public:
  static LoadSampleShapeTest *createSuite() { return new LoadSampleShapeTest(); }
  static void destroySuite(LoadSampleShapeTest *suite) { delete suite; }

  void testInit() {

    LoadSampleShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TSM_ASSERT_EQUALS("should be 8 properties here", 8, (size_t)(alg.getProperties().size()));
  }

  void test_output_workspace_has_MeshObject_when_different_from_input_workspace() {
    LoadSampleShape alg;
    loadMeshObject(alg, "cube.stl", false);
  }

  void test_output_workspace_has_MeshObject_when_the_same_as_input_workspace() {
    LoadSampleShape alg;
    loadMeshObject(alg, "cube.stl", true);
  }

  void test_fail_invalid_stl_solid() {
    LoadSampleShape alg;
    loadFailureTest(alg, "invalid_solid.stl");
  }

  void test_off_cube() {
    LoadSampleShape alg;
    auto cube = loadMeshObject(alg, "cube.off", true);
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 0.000001, 0.000001);
  }

  void test_off_L_shape() {
    LoadSampleShape alg;
    auto shape = loadMeshObject(alg, "L_shape.off", true);
    TS_ASSERT(shape->hasValidShape());
    TS_ASSERT_EQUALS(shape->numberOfVertices(), 12);
    TS_ASSERT_EQUALS(shape->numberOfTriangles(), 18);
    TS_ASSERT_DELTA(shape->volume(), 0.000003, 0.000001);
  }

  void test_off_cube_with_comments() {
    LoadSampleShape alg;
    auto cube = loadMeshObject(alg, "cube_with_comments.off", true);
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 0.000001, 0.000001);
  }

  void test_off_colored_cube() {
    // Cube with colored faces should be read normally,
    // except that the colors are ignored.
    LoadSampleShape alg;
    auto cube = loadMeshObject(alg, "colored_cube.off", true);
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 0.000001, 0.000001);
  }

  void test_peak_workspace() {
    LoadSampleShape alg;
    alg.initialize();
    const int npeaks(10);
    PeaksWorkspace_sptr inputWS = WorkspaceCreationHelper::createPeaksWorkspace(npeaks);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", "__dummy_unused");
    alg.setProperty("OutputWorkspace", inputWS);
    alg.setPropertyValue("Filename", "cube.stl");
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    getMeshObject(alg);
  }

  void test_fail_off_invalid_first_line() {
    LoadSampleShape alg;
    loadFailureTest(alg, "invalid_first_line.off");
  }

  void test_fail_off_non_triangular_face() {
    LoadSampleShape alg;
    loadFailureTest(alg, "cube4.off");
  }

  void test_fail_off_wrong_number_of_vertices() {
    LoadSampleShape alg;
    loadFailureTest(alg, "wrong_number_of_vertices.off");
  }

  void test_fail_off_wrong_number_of_triangles() {
    LoadSampleShape alg;
    loadFailureTest(alg, "wrong_number_of_triangles.off");
  }

  void test_fail_off_invalid_vertex() {
    LoadSampleShape alg;
    loadFailureTest(alg, "invalid_vertex.off");
  }

  void test_fail_off_invalid_triangle() {
    LoadSampleShape alg;
    loadFailureTest(alg, "invalid_triangle.off");
  }

  void testLoadWithRotation() {
    LoadSampleShape alg;
    // cylinder in stl is along the z axis
    auto shape = loadMeshObject(alg, "cylinderOffsetZ.stl", false, "90", "0", "0");
    TS_ASSERT_EQUALS(shape->getBoundingBox().yMin(), -0.30);
  }

  void testLoadWithRotationAndTranslation() {
    LoadSampleShape alg;
    // cylinder in stl is along the z axis, length 30cm and radius 2.5cm
    auto shape = loadMeshObject(alg, "cylinderOffsetZ.stl", false, "90", "0", "0", "0,1,0");
    // translation is applied after the rotation
    TS_ASSERT_EQUALS(shape->getBoundingBox().yMin(), -0.29);
  }

private:
  // Create workspaces and add them to algorithm properties
  MatrixWorkspace_sptr prepareWorkspaces(LoadSampleShape &alg, bool outputWsSameAsInputWs) {
    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors, nbins);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", "__dummy_unused");
    if (outputWsSameAsInputWs) {
      alg.setProperty("OutputWorkspace", inputWS);
    }
    return inputWS;
  }

  const MeshObject *loadMeshObject(LoadSampleShape &alg, const std::string &filename, bool outputWsSameAsInputWs,
                                   const std::string &xRotation = "0", const std::string &yRotation = "0",
                                   const std::string &zRotation = "0", const std::string &translation = "0,0,0") {
    alg.initialize();
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Filename", filename));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("xDegrees", xRotation));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("yDegrees", yRotation));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("zDegrees", zRotation));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("TranslationVector", translation));
    prepareWorkspaces(alg, outputWsSameAsInputWs);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    return getMeshObject(alg);
  }

  void loadFailureTest(LoadSampleShape &alg, const std::string &filename) {
    alg.initialize();
    alg.setPropertyValue("Filename", filename);
    prepareWorkspaces(alg, true);
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());
  }

  const MeshObject *getMeshObject(LoadSampleShape &alg) {
    Workspace_sptr ws = alg.getProperty("OutputWorkspace");
    auto ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);
    if (!ei)
      throw std::invalid_argument("Wrong type of input workspace");
    const auto &s(ei->sample());
    auto &obj = s.getShape();
    auto mObj = dynamic_cast<const MeshObject *>(&obj);
    TSM_ASSERT_DIFFERS("Shape is not a mesh object", mObj, nullptr);
    return mObj;
  }

  std::unique_ptr<MeshObject> createCube() {
    const std::vector<uint32_t> faces{0, 1, 2, 0, 3, 1, 0, 2, 4, 2, 1, 5, 2, 5, 4, 6, 1, 3,
                                      6, 5, 1, 4, 5, 6, 7, 3, 0, 0, 4, 7, 7, 6, 3, 4, 6, 7};
    const std::vector<Mantid::Kernel::V3D> vertices{Mantid::Kernel::V3D(-5, -5, -15), Mantid::Kernel::V3D(5, 5, -15),
                                                    Mantid::Kernel::V3D(5, -5, -15),  Mantid::Kernel::V3D(-5, 5, -15),
                                                    Mantid::Kernel::V3D(5, -5, 15),   Mantid::Kernel::V3D(5, 5, 15),
                                                    Mantid::Kernel::V3D(-5, 5, 15),   Mantid::Kernel::V3D(-5, -5, 15)};
    auto cube = std::make_unique<MeshObject>(faces, vertices, Mantid::Kernel::Material());
    return cube;
  }
};

class LoadSampleShapeTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    auto inWs = WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 4);
    alg = setupAlg(inWs);
  }

  void testLoadSampleShapePerformance() {
    for (int i = 0; i < numberOfIterations; ++i) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

private:
  std::unique_ptr<LoadSampleShape> alg;
  const int numberOfIterations = 5;

  std::unique_ptr<LoadSampleShape> setupAlg(const Workspace2D_sptr &inputWS) {
    auto loadAlg = std::make_unique<LoadSampleShape>();
    loadAlg->initialize();
    loadAlg->setChild(true);
    loadAlg->setProperty("InputWorkspace", inputWS);
    loadAlg->setPropertyValue("OutputWorkspace", "__dummy_unused");
    loadAlg->setPropertyValue("Filename", "tube.stl");

    loadAlg->setRethrows(true);
    return loadAlg;
  }
};
