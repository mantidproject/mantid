// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//     NScD Oak Ridge National Laboratory, European Spallation Source
//     & Institut Laue - Langevin
// SPDX - License - Identifier: GPL - 3.0 +
#ifndef LOAD_SHAPETEST_H_
#define LOAD_SHAPETEST_H_

#include "MantidAPI/AnalysisDataService.h"
#include "MantidAPI/FrameworkManager.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/LoadInstrument.h"
#include "MantidDataHandling/LoadSampleShape.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidKernel/OptionalBool.h"
#include "MantidTestHelpers/ComponentCreationHelper.h"
#include "MantidTestHelpers/WorkspaceCreationHelper.h"
#include <cxxtest/TestSuite.h>

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
    TS_ASSERT_DELTA(cube->volume(), 1.0, 0.000001);
  }

  void test_off_L_shape() {
    LoadSampleShape alg;
    auto shape = loadMeshObject(alg, true, "L_shape.off");
    TS_ASSERT(shape->hasValidShape());
    TS_ASSERT_EQUALS(shape->numberOfVertices(), 12);
    TS_ASSERT_EQUALS(shape->numberOfTriangles(), 18);
    TS_ASSERT_DELTA(shape->volume(), 3.0, 0.000001);
  }

  void test_off_cube_with_comments() {
    LoadSampleShape alg;
    auto cube = loadMeshObject(alg, true, "cube_with_comments.off");
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 1.0, 0.000001);
  }

  void test_off_colored_cube() {
    // Cube with colored faces should be read normally,
    // except that the colors are ignored.
    LoadSampleShape alg;
    auto cube = loadMeshObject(alg, true, "colored_cube.off");
    TS_ASSERT(cube->hasValidShape());
    TS_ASSERT_EQUALS(cube->numberOfVertices(), 8);
    TS_ASSERT_EQUALS(cube->numberOfTriangles(), 12);
    TS_ASSERT_DELTA(cube->volume(), 1.0, 0.000001);
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

private:
  // Create workspaces and add them to algorithm properties
  void prepareWorkspaces(LoadSampleShape &alg, bool outputWsSameAsInputWs) {
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
