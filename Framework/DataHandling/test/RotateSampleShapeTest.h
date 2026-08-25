// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include "MantidAPI/Run.h"
#include "MantidAPI/Sample.h"
#include "MantidDataHandling/RotateSampleShape.h"
#include "MantidFrameworkTestHelpers/ComponentCreationHelper.h"
#include "MantidFrameworkTestHelpers/WorkspaceCreationHelper.h"
#include "MantidGeometry/Instrument/Goniometer.h"
#include "MantidGeometry/Objects/MeshObject.h"
#include "MantidGeometry/Objects/ShapeFactory.h"
#include "MantidKernel/Matrix.h"
#include "MantidKernel/V3D.h"
#include <cxxtest/TestSuite.h>

using namespace Mantid::DataHandling;
using namespace Mantid::API;
using namespace Mantid::Geometry;
using Mantid::DataObjects::Workspace2D_sptr;
using Mantid::Kernel::V3D;

class RotateSampleShapeTest : public CxxTest::TestSuite {
public:
  void test_Init() {
    RotateSampleShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
  }

  Workspace2D_sptr getWsWithCSGSampleShape(std::string shapeXml, std::string wsName = "RotSampleShapeTest_ws") {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
    ShapeFactory shapeMaker;
    ws->mutableSample().setShape(shapeMaker.createShape(shapeXml));
    return ws;
  }

  Workspace2D_sptr getWsWithMeshSampleShape(std::unique_ptr<MeshObject> &meshShape,
                                            std::string wsName = "RotSampleShapeTest_ws") {
    Workspace2D_sptr ws = WorkspaceCreationHelper::create2DWorkspace(10, 10);
    AnalysisDataService::Instance().addOrReplace(wsName, ws);
    ws->mutableSample().setShape(std::move(meshShape));
    return ws;
  }

  void assert_fail_when_invalid_params(std::string axisName, std::string paramStr) {
    RotateSampleShape alg;
    alg.setRethrows(true);
    auto shapeXML = ComponentCreationHelper::cappedCylinderXML(0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
    Workspace2D_sptr ws = getWsWithCSGSampleShape(shapeXML);

    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", "RotSampleShapeTest_ws"));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(axisName, paramStr));
    TS_ASSERT_THROWS(alg.execute(), const std::invalid_argument &);
    TS_ASSERT(!alg.isExecuted());
  }

  void test_exec_failures_when_invalid_params() {
    assert_fail_when_invalid_params("Axis1", ",,,,");
    assert_fail_when_invalid_params("Axis2", ", 1.0,2.0,3.0, 1");
    assert_fail_when_invalid_params("Axis3", "10, x,0,0, -1");
    assert_fail_when_invalid_params("Axis4", "10, 1,y,0, -1");
    assert_fail_when_invalid_params("Axis5", "10, 0,0,z, -1");
    assert_fail_when_invalid_params("Axis0", "10, 1.0,2.0,3.0, sense");
    assert_fail_when_invalid_params("Axis1", "30, 1.0,2.0,3.0, 10");
    assert_fail_when_invalid_params("Axis2", "10, 0.00001,0.00001,0.00001, 1");
  }

  Workspace2D_sptr assert_rotatesample_runs_with_given_shape(std::string &shapeXML,
                                                             std::map<std::string, std::string> &properties) {
    Workspace2D_sptr ws = getWsWithCSGSampleShape(shapeXML, properties["Workspace"]);
    RotateSampleShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    for (const auto &pair : properties) {
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(pair.first, pair.second));
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_EQUALS(ws->run().getNumGoniometers(), 1);
    auto ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);
    const auto shape = std::dynamic_pointer_cast<CSGObject>(ei->sample().getShapePtr());
    std::string shapeStr = shape->getShapeXML();
    TS_ASSERT(shapeStr.find("<goniometer") != shapeStr.npos);
    TS_ASSERT(ei->run().getGoniometer().getR() == Mantid::Kernel::Matrix<double>(3, 3, true));

    return ws;
  }

  void test_rotate_cylindrical_sample_shape() {
    auto shapeXML = ComponentCreationHelper::cappedCylinderXML(0.5, 1.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.), "tube");
    std::map<std::string, std::string> algProperties = {
        {"Workspace", "RotSampleShapeTest_ws"}, {"Axis0", "10,1.0,2.0,3.0,1"}, {"Axis3", "50,4.0,5.0,6.0,-1"}};
    assert_rotatesample_runs_with_given_shape(shapeXML, algProperties);
  }

  void test_rotate_hollow_cylindrical_sample_shape() {
    auto shapeXML = ComponentCreationHelper::hollowCylinderXML(0.3, 0.5, 0.5, V3D(0.0, 0.0, 0.0), V3D(0., 1.0, 0.),
                                                               "hollow_cylinder");
    std::map<std::string, std::string> algProperties = {{"Workspace", "RotSampleShapeTest_ws"},
                                                        {"Axis2", " 45 , 1.0 ,   0.0 ,  1.0 , 1 "},
                                                        {"Axis4", " 90 , 0.0 , 1.0 , 1.0 , -1 "}};
    assert_rotatesample_runs_with_given_shape(shapeXML, algProperties);
  }

  void test_rotate_spherical_sample_shape() {
    auto shapeXML = ComponentCreationHelper::sphereXML(0.02, V3D(0, 0, 0), "sphere");
    std::map<std::string, std::string> algProperties = {{"Workspace", "RotSampleShapeTest_ws"},
                                                        {"Axis0", "60, 1.0,2.0,3.0, 1"},
                                                        {"Axis3", "30 , 4.0, 5.0,6.0, -1"},
                                                        {"Axis2", "10 , 1.0, 0.0 , 0.0,  1 "}};
    assert_rotatesample_runs_with_given_shape(shapeXML, algProperties);
  }

  void test_rotate_cuboid_sample_shape() {
    auto shapeXML = ComponentCreationHelper::cuboidXML(0.005, 0.005, 0.0025, {0., 0., 0.}, "cuboid");
    std::map<std::string, std::string> algProperties = {{"Workspace", "RotSampleShapeTest_ws"},
                                                        {"Axis2", "60, 1.0,2.0,3.0, 1"},
                                                        {"Axis3", "30 , 4.0, 5.0,6.0, -1"},
                                                        {"Axis5", "10 , 1.0, 0.0 , 0.0,  1 "}};
    assert_rotatesample_runs_with_given_shape(shapeXML, algProperties);
  }

  Workspace2D_sptr assert_rotatesample_runs_with_mesh_shape(std::unique_ptr<MeshObject> &meshShape,
                                                            std::map<std::string, std::string> &properties) {
    Workspace2D_sptr ws = getWsWithMeshSampleShape(meshShape, properties["Workspace"]);
    RotateSampleShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize())
    TS_ASSERT(alg.isInitialized())

    for (const auto &pair : properties) {
      TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue(pair.first, pair.second));
    }

    TS_ASSERT_THROWS_NOTHING(alg.execute(););
    TS_ASSERT(alg.isExecuted());
    TS_ASSERT_EQUALS(ws->run().getNumGoniometers(), 1);
    auto ei = std::dynamic_pointer_cast<ExperimentInfo>(ws);
    const auto shape = std::dynamic_pointer_cast<MeshObject>(ei->sample().getShapePtr());
    TS_ASSERT(shape != nullptr);
    TS_ASSERT(ei->run().getGoniometer().getR() == Mantid::Kernel::Matrix<double>(3, 3, true));

    return ws;
  }

  std::unique_ptr<MeshObject> createCube(const double size, const V3D &centre) {
    /**
     * Create cube of side length size with specified centre,
     * parellel to axes and non-negative vertex coordinates.
     */
    double min = 0.0 - 0.5 * size;
    double max = 0.5 * size;
    std::vector<V3D> vertices;
    vertices.emplace_back(centre + V3D(max, max, max));
    vertices.emplace_back(centre + V3D(min, max, max));
    vertices.emplace_back(centre + V3D(max, min, max));
    vertices.emplace_back(centre + V3D(min, min, max));
    vertices.emplace_back(centre + V3D(max, max, min));
    vertices.emplace_back(centre + V3D(min, max, min));
    vertices.emplace_back(centre + V3D(max, min, min));
    vertices.emplace_back(centre + V3D(min, min, min));

    std::vector<uint32_t> triangles;
    // top face of cube - z max
    triangles.insert(triangles.end(), {0, 1, 2});
    triangles.insert(triangles.end(), {2, 1, 3});
    // right face of cube - x max
    triangles.insert(triangles.end(), {0, 2, 4});
    triangles.insert(triangles.end(), {4, 2, 6});
    // back face of cube - y max
    triangles.insert(triangles.end(), {0, 4, 1});
    triangles.insert(triangles.end(), {1, 4, 5});
    // bottom face of cube - z min
    triangles.insert(triangles.end(), {7, 5, 6});
    triangles.insert(triangles.end(), {6, 5, 4});
    // left face of cube - x min
    triangles.insert(triangles.end(), {7, 3, 5});
    triangles.insert(triangles.end(), {5, 3, 1});
    // front fact of cube - y min
    triangles.insert(triangles.end(), {7, 6, 3});
    triangles.insert(triangles.end(), {3, 6, 2});

    // Use efficient constructor
    std::unique_ptr<MeshObject> retVal =
        std::make_unique<MeshObject>(std::move(triangles), std::move(vertices), Mantid::Kernel::Material());
    return retVal;
  }

  void test_rotate_mesh_cuboid_sample_shape() {
    auto cuboidMeshShape = createCube(2, V3D(0, 0, 0));
    std::map<std::string, std::string> algProperties = {{"Workspace", "RotSampleShapeTest_ws"},
                                                        {"Axis2", "60, 1.0,2.0,3.0, 1"},
                                                        {"Axis3", "30 , 4.0, 5.0,6.0, -1"},
                                                        {"Axis5", "10 , 1.0, 0.0 , 0.0,  1 "}};
    assert_rotatesample_runs_with_mesh_shape(cuboidMeshShape, algProperties);
  }

  // ~~~~ What the algorithm actually does to the geometry ~~~~
  //
  // Everything above asserts only that the algorithm ran, that a <goniometer> tag appeared and that
  // the shape is still of the type it started as. Nothing checked that a point moved, so a sign
  // error or a swapped axis order would pass the whole suite. The tests below pin the geometry, and
  // the ones marked CHARACTERISATION pin behaviour that is about to change, so that the change
  // shows up as an edit to an assertion rather than disappearing into a file that asserted nothing.

  static Mantid::Kernel::Matrix<double> ninetyAboutZ() {
    return Mantid::Kernel::Matrix<double>(std::vector<double>{0, -1, 0, 1, 0, 0, 0, 0, 1});
  }

  /// Rotate a sphere centred on +x, so that where it ends up says what rotation was applied.
  Workspace2D_sptr rotateSphere(const std::string &wsName, const std::string &axis,
                                const Mantid::Kernel::Matrix<double> &runGoniometer) {
    auto shapeXML = ComponentCreationHelper::sphereXML(0.5, V3D(1.0, 0.0, 0.0), "sphere");
    Workspace2D_sptr ws = getWsWithCSGSampleShape(shapeXML, wsName);
    ws->mutableRun().mutableGoniometer().setR(runGoniometer);
    runRotateSampleShape(wsName, axis);
    return ws;
  }

  void runRotateSampleShape(const std::string &wsName, const std::string &axis) {
    RotateSampleShape alg;
    TS_ASSERT_THROWS_NOTHING(alg.initialize());
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Workspace", wsName));
    TS_ASSERT_THROWS_NOTHING(alg.setPropertyValue("Axis0", axis));
    TS_ASSERT_THROWS_NOTHING(alg.execute());
    TS_ASSERT(alg.isExecuted());
  }

  void test_csg_surfaces_are_actually_rotated() {
    // a sphere centred on +x, turned 90 degrees anticlockwise about z, has to end up on +y
    auto ws = rotateSphere("RotSampleShapeTest_geom", "90,0,0,1,1", Mantid::Kernel::Matrix<double>(3, 3, true));

    const auto &shape = ws->sample().getShape();
    TS_ASSERT(shape.isValid(V3D(0.0, 1.0, 0.0)));
    TS_ASSERT(!shape.isValid(V3D(1.0, 0.0, 0.0)));
  }

  void test_run_goniometer_is_currently_folded_into_the_shape() {
    // CHARACTERISATION: the algorithm reads the run's goniometer and multiplies its own rotation by
    // it, so a 90 degree request against a 90 degree run goniometer turns the sphere a full 180 and
    // lands it on -x. Rotating the sample shape and enacting the run's orientation are two separate
    // jobs, and doing both here is what makes a user rotation indistinguishable from a bake.
    auto ws = rotateSphere("RotSampleShapeTest_gonio", "90,0,0,1,1", ninetyAboutZ());

    const auto &shape = ws->sample().getShape();
    TS_ASSERT(shape.isValid(V3D(-1.0, 0.0, 0.0)));
    TS_ASSERT(!shape.isValid(V3D(0.0, 1.0, 0.0)));
  }

  void test_two_calls_on_a_csg_shape_currently_overwrite() {
    // CHARACTERISATION: addGoniometerTag replaces the tag outright, so the second call discards the
    // first and the sphere is left turned 90 degrees rather than 180. A mesh shape composes, so the
    // two shape types already disagree here.
    auto ws = rotateSphere("RotSampleShapeTest_twice", "90,0,0,1,1", Mantid::Kernel::Matrix<double>(3, 3, true));
    runRotateSampleShape("RotSampleShapeTest_twice", "90,0,0,1,1");

    const auto &shape = ws->sample().getShape();
    TS_ASSERT(shape.isValid(V3D(0.0, 1.0, 0.0)));
    TS_ASSERT(!shape.isValid(V3D(-1.0, 0.0, 0.0)));
  }

  void test_mesh_shape_currently_folds_in_the_run_goniometer_too() {
    // CHARACTERISATION: the mesh branch composes the same product onto the vertices, so an offset
    // cube spanning x = 4..6 is carried right round to x = -6..-4.
    auto cube = createCube(2, V3D(5.0, 0.0, 0.0));
    Workspace2D_sptr ws = getWsWithMeshSampleShape(cube, "RotSampleShapeTest_mesh_gonio");
    ws->mutableRun().mutableGoniometer().setR(ninetyAboutZ());

    runRotateSampleShape("RotSampleShapeTest_mesh_gonio", "90,0,0,1,1");

    const auto &box = ws->sample().getShape().getBoundingBox();
    TS_ASSERT_DELTA(box.xMin(), -6.0, 1e-8);
    TS_ASSERT_DELTA(box.xMax(), -4.0, 1e-8);
  }

  void test_csg_rotation_is_currently_reported_as_a_goniometer_bake() {
    // CHARACTERISATION, and the defect this whole change exists to fix. The user's rotation is
    // written into <goniometer>, and with nothing beside it to say how much of that tag is a bake
    // the whole of it is taken to be one. So a shape the user deliberately turned within its own
    // frame claims to have been moved into the lab frame, and a caller that then asks how much of
    // the run's goniometer is still outstanding gets the wrong answer.
    auto ws = rotateSphere("RotSampleShapeTest_marker", "90,0,0,1,1", Mantid::Kernel::Matrix<double>(3, 3, true));

    TS_ASSERT_EQUALS(ws->sample().getShape().getAppliedRotation(), ninetyAboutZ());
  }

  void test_mesh_rotation_does_not_claim_the_shape_has_been_baked() {
    // The mesh branch already gets this right: rotating vertices re-expresses the shape within its
    // own frame and leaves the frame marker alone. This is an invariant, not characterisation.
    auto cube = createCube(2, V3D(5.0, 0.0, 0.0));
    Workspace2D_sptr ws = getWsWithMeshSampleShape(cube, "RotSampleShapeTest_mesh_marker");

    runRotateSampleShape("RotSampleShapeTest_mesh_marker", "90,0,0,1,1");

    TS_ASSERT_EQUALS(ws->sample().getShape().getAppliedRotation(), Mantid::Kernel::Matrix<double>(3, 3, true));
  }
};
