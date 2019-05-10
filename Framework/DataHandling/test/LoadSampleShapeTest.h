// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOAD_SHAPETEST_H_
#define LOAD_SHAPETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FileFinder.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadBinaryStl.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"

#include <cxxtest/TestSuite.h>
#include <numeric>

using namespace Mantid;
using namespace Mantid::API;
using namespace Mantid::DataHandling;
using namespace Mantid::DataObjects;
using namespace Mantid::Geometry;

class LoadSampleShapeTest : public CxxTest::TestSuite {
public:
  static LoadSampleShapeTest *createSuite() {
    return new LoadSampleShapeTest();
  }
  static void destroySuite(LoadSampleShapeTest *suite) { delete suite; }

  void testInit() {

    LoadSampleShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT(alg.isInitialized());

    TSM_ASSERT_EQUALS("should be 4 properties here", 4,
                      (size_t)(alg.getProperties().size()));
  }

  void
  test_output_workspace_has_MeshObject_when_different_from_input_workspace() {
    LoadSampleShape alg;
    loadMeshObject(alg, false, "cube.stl");
  }

  void test_output_workspace_has_MeshObject_when_the_same_as_input_workspace() {
    LoadSampleShape alg;
    loadMeshObject(alg, true, "cube.stl");
  }

  void test_fail_invalid_stl_solid() {
    LoadSampleShape alg;
    loadFailureTest(alg, "invalid_solid.stl");
  }

  void test_off_cube() {
    LoadSampleShape alg;
    auto cube = loadMeshObject(alg, true, "cube.off");
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 0.000001, 0.000001);
  }

  void test_off_L_shape() {
    LoadSampleShape alg;
    auto shape = loadMeshObject(alg, true, "L_shape.off");
    TS_ASSERT(shape->hasValidShape());
    TS_ASSERT_EQUALS(shape->numberOfVertices(), 12);
    TS_ASSERT_EQUALS(shape->numberOfTriangles(), 18);
    TS_ASSERT_DELTA(shape->volume(), 0.000003, 0.000001);
  }

  void test_off_cube_with_comments() {
    LoadSampleShape alg;
    auto cube = loadMeshObject(alg, true, "cube_with_comments.off");
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 0.000001, 0.000001);
  }

  void test_off_colored_cube() {
    // Cube with colored faces should be read normally,
    // except that the colors are ignored.
    LoadSampleShape alg;
    auto cube = loadMeshObject(alg, true, "colored_cube.off");
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 0.000001, 0.000001);
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

  void testXRotation() {
    LoadSampleShape alg;
    alg.initialize();
    MatrixWorkspace_sptr inputWS = prepareWorkspaces(alg, true);
    inputWS->mutableRun().mutableGoniometer().pushAxis("Axis0", 1, 0, 0, 45);

    auto sampleMesh= loadCube();
    alg.rotate(*sampleMesh, inputWS);
    std::vector<double> rotatedVertices = sampleMesh->getVertices();
    std::vector<double> vectorToMatch = {
        -5, 7.07106,    -14.142136, 5,  14.142136,  -7.07106,
        5,  7.07106,    -14.142136, -5, 14.142136,  -7.07106,
        5,  -14.142136, 7.07106,    5,  -7.07106,   14.142136,
        -5, -7.07106,   14.142136,  -5, -14.142136, 7.07106};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }
  void testYRotation() {
    LoadSampleShape alg;
    alg.initialize();
    MatrixWorkspace_sptr inputWS = prepareWorkspaces(alg, true);
    inputWS->mutableRun().mutableGoniometer().pushAxis("Axis0", 0, 1, 0, 90);
    auto sampleMesh= loadCube();
    alg.rotate(*sampleMesh, inputWS);
    std::vector<double> rotatedVertices = sampleMesh->getVertices();
    std::vector<double> vectorToMatch = {-15, -5,  5,  -15, 5,  -5, -15, -5,
                                         -5,  -15, 5,  5,   15, -5, -5,  15,
                                         5,   -5,  15, 5,   5,  15, -5,  5};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testZRotation() {
    LoadSampleShape alg;
    alg.initialize();
    MatrixWorkspace_sptr inputWS = prepareWorkspaces(alg, true);
    inputWS->mutableRun().mutableGoniometer().pushAxis("Axis0", 0, 0, 1, 180);
    auto sampleMesh = loadCube();
    alg.rotate(*sampleMesh, inputWS);
    std::vector<double> rotatedVertices = sampleMesh->getVertices();
    std::vector<double> vectorToMatch = {5,   5,  -15, -5,  -5, -15, -5, 5,
                                         -15, 5,  -5,  -15, -5, 5,   15, -5,
                                         -5,  15, 5,   -5,  15, 5,   5,  15};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

  void testMultiRotation() {
    LoadSampleShape alg;
    alg.initialize();
    MatrixWorkspace_sptr inputWS = prepareWorkspaces(alg, true);
    inputWS->mutableRun().mutableGoniometer().pushAxis("Z", 0, 0, 1, 35);
    inputWS->mutableRun().mutableGoniometer().pushAxis("Y", 0, 1, 0, 20);
    inputWS->mutableRun().mutableGoniometer().pushAxis("X", 1, 0, 0, 70);
    auto sampleMesh= loadCube();
    alg.rotate(*sampleMesh, inputWS);
    std::vector<double> rotatedVertices = sampleMesh->getVertices();
    std::vector<double> vectorToMatch = {
        -13.70635, 5.52235,   -7.52591,  -5.33788,  15.55731,  -2.11589,
        -6.00884,  10.91220,  -10.94611, -13.03539, 10.16745,  1.30430,
        13.03539,  -10.16745, -1.30430,  13.70635,  -5.52235,  7.52591,
        6.00884,   -10.91220, 10.94611,  5.33788,   -15.55731, 2.11589};
    for (size_t i = 0; i < 24; ++i) {
      TS_ASSERT_DELTA(rotatedVertices[i], vectorToMatch[i], 1e-5);
    }
  }

private:
  // Create workspaces and add them to algorithm properties
  MatrixWorkspace_sptr prepareWorkspaces(LoadSampleShape &alg,
                                         bool outputWsSameAsInputWs) {
    const int nvectors(2), nbins(10);
    MatrixWorkspace_sptr inputWS =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(nvectors,
                                                                     nbins);
    alg.setChild(true);
    alg.setProperty("InputWorkspace", inputWS);
    alg.setPropertyValue("OutputWorkspace", "__dummy_unused");
    if (outputWsSameAsInputWs) {
      alg.setProperty("OutputWorkspace", inputWS);
    }
    return inputWS;
  }

  const MeshObject *loadMeshObject(LoadSampleShape &alg,
                                   bool outputWsSameAsInputWs,
                                   const std::string filename) {
    alg.initialize();
    alg.setPropertyValue("Filename", filename);
    prepareWorkspaces(alg, outputWsSameAsInputWs);
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
    return getMeshObject(alg);
  }

  void loadFailureTest(LoadSampleShape &alg, const std::string filename) {
    alg.initialize();
    alg.setPropertyValue("Filename", filename);
    prepareWorkspaces(alg, true);
    TS_ASSERT_THROWS_ANYTHING(alg.execute());
    TS_ASSERT(!alg.isExecuted());
  }

  const MeshObject *getMeshObject(LoadSampleShape &alg) {
    MatrixWorkspace_sptr ws = alg.getProperty("OutputWorkspace");
    const auto &s(ws->sample());
    auto &obj = s.getShape();
    auto mObj = dynamic_cast<const MeshObject *>(&obj);
    TSM_ASSERT_DIFFERS("Shape is not a mesh object", mObj, nullptr);
    return mObj;
  }

  std::unique_ptr<MeshObject> loadCube() {
    std::string path = FileFinder::Instance().getFullPath("cubeBin.stl");
    constexpr ScaleUnits unit = ScaleUnits::metres;
    auto loader = LoadBinaryStl(path, unit);
    auto cube = loader.readStl();
    return cube;
  }
};

class LoadSampleShapeTestPerformance : public CxxTest::TestSuite {
public:
  void setUp() override {
    auto inWs =
        WorkspaceCreationHelper::create2DWorkspaceWithFullInstrument(10, 4);
    alg = setupAlg(inWs);
  }

  void testLoadSampleShapePerformance() {
    for (int i = 0; i < numberOfIterations; ++i) {
      TS_ASSERT_THROWS_NOTHING(alg->execute());
    }
  }

private:
  LoadSampleShape *alg;
  const int numberOfIterations = 5;

  LoadSampleShape *setupAlg(Workspace2D_sptr inputWS) {
    LoadSampleShape *loadAlg = new LoadSampleShape();
    loadAlg->initialize();
    loadAlg->setChild(true);
    loadAlg->setProperty("InputWorkspace", inputWS);
    loadAlg->setPropertyValue("OutputWorkspace", "__dummy_unused");
    loadAlg->setPropertyValue("Filename", "tube.stl");

    loadAlg->setRethrows(true);
    return loadAlg;
  }
};
#endif /* LOAD_SHAPETEST_H_ */
