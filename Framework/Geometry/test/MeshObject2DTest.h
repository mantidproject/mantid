// Mantid Repository : https://github.com/mantidproject/mantid
//
// Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
//   NScD Oak Ridge National Laboratory, European Spallation Source,
//   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
// SPDX - License - Identifier: GPL - 3.0 +
#pragma once

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MockRNG.h"
#include <cmath>

using Mantid::Geometry::MeshObject2D;
using Mantid::Kernel::V3D;

namespace {
MeshObject2D makeSimpleTriangleMesh() {
  std::vector<V3D> vertices;
  vertices.emplace_back(V3D(-1, 0, 0));
  vertices.emplace_back(V3D(1, 0, 0));
  vertices.emplace_back(V3D(0, 1, 0));
  std::vector<uint32_t> triangles;
  triangles.insert(triangles.end(), {0, 1, 2});
  return MeshObject2D(triangles, vertices, Mantid::Kernel::Material());
}
MeshObject2D makeTrapezoidMesh(const V3D &a, const V3D &b, const V3D &c, const V3D &d) {
  std::vector<V3D> vertices{a, b, c, d};
  std::vector<uint32_t> triangles;
  triangles.insert(triangles.end(), {0, 1, 2});
  triangles.insert(triangles.end(), {2, 3, 0});
  return MeshObject2D(triangles, vertices, Mantid::Kernel::Material());
}
} // namespace

class MeshObject2DTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MeshObject2DTest *createSuite() { return new MeshObject2DTest(); }
  static void destroySuite(MeshObject2DTest *suite) { delete suite; }

  void test_not_in_plane_if_insufficient_points() {
    std::vector<V3D> points;
    points.emplace_back(V3D{1, 0, 0});
    points.emplace_back(V3D{2, 1, 0});
    TS_ASSERT(!MeshObject2D::pointsCoplanar(points));
  }

  void test_points_not_in_plane_if_colinear() {
    std::vector<V3D> points;
    points.emplace_back(V3D{1, 0, 0});
    points.emplace_back(V3D{2, 0, 0});
    points.emplace_back(V3D{3, 0, 0});
    TS_ASSERT(!MeshObject2D::pointsCoplanar(points));
  }

  void test_points_in_plane() {
    using Mantid::Kernel::V3D;
    std::vector<V3D> points;
    points.emplace_back(V3D{1, 0, 0});
    points.emplace_back(V3D{2, 0, 0});
    points.emplace_back(V3D{3, 0, 0});
    points.emplace_back(V3D{1, 1, 0});
    TS_ASSERT(MeshObject2D::pointsCoplanar(points));
  }

  void test_points_not_in_plane() {
    std::vector<V3D> points;
    // Make tetrahedron
    points.emplace_back(V3D{-1, 0, 0});
    points.emplace_back(V3D{1, 0, 0});
    points.emplace_back(V3D{0, 0, -1});
    points.emplace_back(V3D{0, 1, 0});
    TS_ASSERT(!MeshObject2D::pointsCoplanar(points));
  }
  void test_construct_with_insufficient_points_throws() {
    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(0, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));

    std::vector<uint32_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 1}); // invalid, but doesn't matter for this test

    // Test constructor taking lvalue references
    TSM_ASSERT_THROWS("Too few points, should throw", MeshObject2D(triangles, vertices, Mantid::Kernel::Material()),
                      std::invalid_argument &);

    // Test constructor taking rvalue references
    TSM_ASSERT_THROWS("Too few points, should throw",
                      MeshObject2D(std::move(triangles), std::move(vertices), Mantid::Kernel::Material()),
                      std::invalid_argument &);
  }

  void test_construct_with_colinear_points_throws() {
    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(0, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(2, 0, 0));

    std::vector<uint32_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2});

    // Test constructor taking lvalue references
    TSM_ASSERT_THROWS("Colinear points, should throw", MeshObject2D(triangles, vertices, Mantid::Kernel::Material()),
                      std::invalid_argument &);

    // Test constructor taking rvalue references
    TSM_ASSERT_THROWS("Colinear points, should throw",
                      MeshObject2D(std::move(triangles), std::move(vertices), Mantid::Kernel::Material()),
                      std::invalid_argument &);
  }

  void test_construct_with_non_coplanar_points_throws() {
    std::vector<V3D> vertices;
    // Verices are not in a plane
    vertices.emplace_back(V3D(0, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    vertices.emplace_back(V3D(1, 1, 0));
    vertices.emplace_back(V3D(1, 0, 1));

    std::vector<uint32_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2});

    // Test constructor taking lvalue references
    TSM_ASSERT_THROWS("non-coplanar points, should throw",
                      MeshObject2D(triangles, vertices, Mantid::Kernel::Material()), std::invalid_argument &);

    // Test constructor taking rvalue references
    TSM_ASSERT_THROWS("non-coplanar points, should throw",
                      MeshObject2D(std::move(triangles), std::move(vertices), Mantid::Kernel::Material()),
                      std::invalid_argument &);
  }

  void test_construct() {
    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(-1, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    std::vector<uint32_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2});
#
    MeshObject2D mesh(triangles, vertices, Mantid::Kernel::Material());
    TS_ASSERT(mesh.hasValidShape());
    TS_ASSERT_EQUALS(mesh.volume(), 0);
  }

  void test_isValid() {
    auto mesh = makeSimpleTriangleMesh();
    TS_ASSERT(mesh.isValid(V3D{0, 0.5, 0}));
    TS_ASSERT(mesh.isValid(V3D{-1, 0, 0}));
    TS_ASSERT(mesh.isValid(V3D{1, 0, 0}));
    TS_ASSERT(mesh.isValid(V3D{0, 1, 0}));
    TS_ASSERT(!mesh.isValid(V3D{0, 0.5, 1}));
  }

  void test_distanceToPlane() {
    auto mesh = makeSimpleTriangleMesh();
    TS_ASSERT_EQUALS(0, mesh.distanceToPlane(V3D{0, 0.5, 0}));
    TS_ASSERT_EQUALS(0, mesh.distanceToPlane(V3D{-1, 0, 0}));
    TS_ASSERT_EQUALS(0, mesh.distanceToPlane(V3D{1, 0, 0}));
    TS_ASSERT_EQUALS(0, mesh.distanceToPlane(V3D{0, 1, 0}));
    TS_ASSERT_EQUALS(1.0, mesh.distanceToPlane(V3D{0, 0.5, 1}));
  }

  void test_solidAngle_side_on() {
    using namespace Mantid::Kernel;
    auto mesh = makeSimpleTriangleMesh();
    auto solidAngle = mesh.solidAngle(V3D{0, 2, 0}); // observer is in plane of triangle, outside the triangle
    TS_ASSERT_EQUALS(solidAngle, 0);                 // seen side-on solid angle is 0
  }
  void test_square_solid_angle() {

    // Unit square inside unit cube. Any cube face will have solid angle 1/6th
    // of total 4pi steradians. Observer at origin.

    double expected = 2.0 * M_PI / 3.0; //  4pi/6
    // Unit square at distance cos(M_PI / 4) from observer
    double unitSphereRadius = 1;
    double halfSideLength = unitSphereRadius * sin(M_PI / 4);
    double observerDistance = unitSphereRadius * cos(M_PI / 4);
    std::vector<V3D> vertices = {
        V3D{-halfSideLength, -halfSideLength, observerDistance}, V3D{-halfSideLength, halfSideLength, observerDistance},
        V3D{halfSideLength, halfSideLength, observerDistance}, V3D{halfSideLength, -halfSideLength, observerDistance}};
    std::vector<uint32_t> triangles{2, 1, 0, 0, 3, 2};
    MeshObject2D mesh(triangles, vertices, Mantid::Kernel::Material{});
    double solidAngle = mesh.solidAngle(V3D{0, 0, 0});
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);

    // Only the positive solid angle is counted. Observe from the other side and
    // solid angle is zero.
    solidAngle = mesh.solidAngle(V3D{0, 0, 2 * observerDistance});
    TS_ASSERT_DELTA(solidAngle, 0, 1e-3);
  }

  void test_solidAngle_scaled() {
    // Unit square inside unit cube. Any cube face will have solid angle 1/6th
    // of total 4pi steradians. Observer at origin.

    double expected = 2.0 * M_PI / 3.0; //  4pi/6
    // Unit square at distance 0.5 from observer
    double unitSphereRadius = 1;
    double halfSideLength = unitSphereRadius * sin(M_PI / 4);
    double observerDistance = unitSphereRadius * cos(M_PI / 4);
    std::vector<V3D> vertices = {
        V3D{-halfSideLength, -halfSideLength, observerDistance}, V3D{-halfSideLength, halfSideLength, observerDistance},
        V3D{halfSideLength, halfSideLength, observerDistance}, V3D{halfSideLength, -halfSideLength, observerDistance}};
    std::vector<uint32_t> triangles{2, 1, 0, 0, 3, 2};
    // Scaling square uniformly (and reducing distance to origin by same
    // factor), yields same angular area 4pi/6
    V3D scaleFactor{0.5, 0.5, 0.5};
    MeshObject2D mesh(triangles, vertices, Mantid::Kernel::Material{});
    double solidAngle = mesh.solidAngle(V3D{0, 0, 0}, scaleFactor);
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);

    // Scaling square uniformly (and increasing distance to origin by same
    // factor), yields same angular area 4pi/6
    scaleFactor = {2, 2, 2};
    solidAngle = mesh.solidAngle(V3D{0, 0, 0}, scaleFactor);
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);
  }

  void test_isValid_multi_triangle() {

    // Make 2 Triangles bounded by the specified V3Ds
    auto mesh = makeTrapezoidMesh(V3D{0, 0, 0}, V3D{0, 1, 0}, V3D{1, 1, 0}, V3D{1, 0, 0});
    double delta = 1e-6;
    TS_ASSERT(mesh.isValid(V3D{0, 0, 0}));
    TS_ASSERT(mesh.isValid(V3D{0.5, 0.5, 0}));
    TS_ASSERT(mesh.isValid(V3D{0, 0, 0}));
    TSM_ASSERT("Just outside", !mesh.isValid(V3D{0 - delta, 0, 0}));
    TS_ASSERT(mesh.isValid(V3D{1, 1, 0}));
    TSM_ASSERT("Just outside", !mesh.isValid(V3D{1, 1 + delta, 0}));
  }

  void test_interceptSurface() {

    auto mesh = makeSimpleTriangleMesh();

    // Track goes through triangle body
    Mantid::Geometry::Track onTargetTrack(V3D{0.5, 0.5, -1}, V3D{0, 0, 1} /*along z*/);
    TS_ASSERT_EQUALS(mesh.interceptSurface(onTargetTrack), 1);
    TS_ASSERT_EQUALS(onTargetTrack.count(), 1);

    // Track completely misses
    Mantid::Geometry::Track missTargetTrack(V3D{50, 0.5, -1},
                                            V3D{0, 0, 1} /*along z*/); // Intersects plane but no triangles
    TS_ASSERT_EQUALS(mesh.interceptSurface(missTargetTrack), 0);
    TS_ASSERT_EQUALS(missTargetTrack.count(), 0);

    // Track goes through edge
    Mantid::Geometry::Track edgeTargetTrack(V3D{0, 0, -1}, V3D{0, 0, 1} /*along z*/); // Passes through lower edge
    TS_ASSERT_EQUALS(mesh.interceptSurface(edgeTargetTrack), 1);
    TS_ASSERT_EQUALS(edgeTargetTrack.count(), 1);
  }

  void test_equals() {

    auto a = makeTrapezoidMesh(V3D{0, 0, 0}, V3D{0, 1, 0}, V3D{1, 1, 0}, V3D{1, 0, 0});
    auto b = makeTrapezoidMesh(V3D{0, 0, 0}, V3D{0, 1, 0}, V3D{1, 1, 0}, V3D{1, 0, 0});
    auto c = makeTrapezoidMesh(V3D{0.1, 0, 0}, V3D{0, 1, 0}, V3D{1, 1, 0}, V3D{1, 0, 0});
    TS_ASSERT_EQUALS(a, b);
    TS_ASSERT_DIFFERS(a, c);
  }

  void test_clone() {
    auto mesh = makeSimpleTriangleMesh();
    auto clone = std::unique_ptr<MeshObject2D>(mesh.clone());
    TS_ASSERT_EQUALS(*clone, mesh);
  }

  void test_clone_with_material() {
    using namespace Mantid::Kernel;
    auto a = makeSimpleTriangleMesh();
    // Use a different material
    auto b = std::unique_ptr<MeshObject2D>(
        a.cloneWithMaterial(Material("hydrogen", Material::parseChemicalFormula("H"), 3)));
    TS_ASSERT_DIFFERS(a, *b);
    // Use same material
    auto c = std::unique_ptr<MeshObject2D>(a.cloneWithMaterial(Material{})); // same empty material
    TS_ASSERT_EQUALS(a, *c);
  }

  void test_boundingBox() {
    auto mesh = makeSimpleTriangleMesh(); // Lies in z plane
    auto boundingBox = mesh.getBoundingBox();
    TS_ASSERT_EQUALS(boundingBox.zMin(), 0);
    TS_ASSERT_DELTA(boundingBox.zMax(), boundingBox.zMin(), MeshObject2D::MinThickness);
    TS_ASSERT_EQUALS(boundingBox.xMin(), -1);
    TS_ASSERT_EQUALS(boundingBox.xMax(), 1);
    TS_ASSERT_EQUALS(boundingBox.yMin(), 0);
    TS_ASSERT_EQUALS(boundingBox.yMax(), 1);
  }

  // Characterisation test.
  void test_generatePointInObject_not_supported() {

    // Generating points in a 3D bounding box volume does not make sense for a
    // plane
    auto mesh = makeSimpleTriangleMesh();
    testing::NiceMock<MockRNG> generator;
    std::optional<V3D> point;
    TS_ASSERT_THROWS(point = mesh.generatePointInObject(generator, 10), std::runtime_error &);
  }
  // Characterisation test.
  void test_generatePointInObject_with_active_region_not_supported() {

    // Generating points in a 3D bounding box volume does not make sense for a
    // plane
    auto mesh = makeSimpleTriangleMesh();
    testing::NiceMock<MockRNG> generator;
    Mantid::Geometry::BoundingBox boundingBox;
    std::optional<V3D> point;
    TS_ASSERT_THROWS(point = mesh.generatePointInObject(generator, boundingBox, 10), std::runtime_error &);
  }

  // Characterisation test.
  void test_GetObjGeom_not_implemented() {
    auto mesh = makeSimpleTriangleMesh();
    std::vector<V3D> vectors;
    double radius, height, innerRadius;
    Mantid::Geometry::detail::ShapeInfo::GeometryShape shape;

    TS_ASSERT_THROWS(mesh.GetObjectGeom(shape, vectors, innerRadius, radius, height), std::runtime_error &);
  }

  void test_get_material() {
    auto mesh = makeSimpleTriangleMesh();
    TS_ASSERT_EQUALS(mesh.material().name(), Mantid::Kernel::Material{}.name())
  }

  void test_id() {
    auto mesh = makeSimpleTriangleMesh();
    TS_ASSERT_EQUALS(mesh.id(), MeshObject2D::Id);
  }

  void test_get_geometry_hanlder_not_implemented() {
    auto mesh = makeSimpleTriangleMesh();

    auto handler = mesh.getGeometryHandler();
    // basic sanity checks
    TS_ASSERT_EQUALS(handler->numberOfTriangles(), 1);
    TS_ASSERT_EQUALS(handler->numberOfPoints(), 3);
  }
};
