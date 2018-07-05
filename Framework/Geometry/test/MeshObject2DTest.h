#ifndef MANTID_GEOMETRY_MESHOBJECT2DTEST_H_
#define MANTID_GEOMETRY_MESHOBJECT2DTEST_H_

#include <cxxtest/TestSuite.h>
#include <gmock/gmock.h>

#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"
#include "MantidGeometry/Objects/BoundingBox.h"
#include "MantidGeometry/Rendering/ShapeInfo.h"
#include "MantidGeometry/Rendering/GeometryHandler.h"
#include "MantidKernel/PseudoRandomNumberGenerator.h"
#include "MantidKernel/WarningSuppressions.h"
#include "MantidKernel/Material.h"
#include <cmath>

using Mantid::Geometry::MeshObject2D;
using Mantid::Kernel::V3D;

namespace {
MeshObject2D makeSimpleTriangleMesh() {
  std::vector<V3D> vertices;
  vertices.emplace_back(V3D(-1, 0, 0));
  vertices.emplace_back(V3D(1, 0, 0));
  vertices.emplace_back(V3D(0, 1, 0));
  std::vector<uint16_t> triangles;
  triangles.insert(triangles.end(), {0, 1, 2});
  return MeshObject2D(triangles, vertices, Mantid::Kernel::Material());
}
MeshObject2D makeTrapezoidMesh(const V3D &a, const V3D &b, const V3D &c,
                               const V3D &d) {
  std::vector<V3D> vertices{a, b, c, d};
  std::vector<uint16_t> triangles;
  triangles.insert(triangles.end(), {0, 1, 2});
  triangles.insert(triangles.end(), {2, 3, 0});
  return MeshObject2D(triangles, vertices, Mantid::Kernel::Material());
}
// -----------------------------------------------------------------------------
// Mock Random Number Generator
// -----------------------------------------------------------------------------
class MockRNG : public Mantid::Kernel::PseudoRandomNumberGenerator {
public:
  GCC_DIAG_OFF_SUGGEST_OVERRIDE
  MOCK_METHOD0(nextValue, double());
  MOCK_METHOD2(nextValue, double(double, double));
  MOCK_METHOD2(nextInt, int(int, int));
  MOCK_METHOD0(restart, void());
  MOCK_METHOD0(save, void());
  MOCK_METHOD0(restore, void());
  MOCK_METHOD1(setSeed, void(size_t));
  MOCK_METHOD2(setRange, void(const double, const double));
  MOCK_CONST_METHOD0(min, double());
  MOCK_CONST_METHOD0(max, double());
  GCC_DIAG_ON_SUGGEST_OVERRIDE
};
}

class MeshObject2DTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MeshObject2DTest *createSuite() { return new MeshObject2DTest(); }
  static void destroySuite(MeshObject2DTest *suite) { delete suite; }

  void test_construct_with_insufficient_points_throws() {
    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(0, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));

    std::vector<uint16_t> triangles;
    triangles.insert(triangles.end(),
                     {0, 1, 1}); // invalid, but doesn't matter for this test

    // Test constructor taking lvalue references
    TSM_ASSERT_THROWS(
        "Too few points, should throw",
        MeshObject2D(triangles, vertices, Mantid::Kernel::Material()),
        std::invalid_argument &);

    // Test constructor taking rvalue references
    TSM_ASSERT_THROWS("Too few points, should throw",
                      MeshObject2D(std::move(triangles), std::move(vertices),
                                   Mantid::Kernel::Material()),
                      std::invalid_argument &);
  }

  void test_construct_with_colinear_points_throws() {
    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(0, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(2, 0, 0));

    std::vector<uint16_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2});

    // Test constructor taking lvalue references
    TSM_ASSERT_THROWS(
        "Colinear points, should throw",
        MeshObject2D(triangles, vertices, Mantid::Kernel::Material()),
        std::invalid_argument &);

    // Test constructor taking rvalue references
    TSM_ASSERT_THROWS("Colinear points, should throw",
                      MeshObject2D(std::move(triangles), std::move(vertices),
                                   Mantid::Kernel::Material()),
                      std::invalid_argument &);
  }

  void test_construct_with_non_coplanar_points_throws() {
    std::vector<V3D> vertices;
    // Verices are not in a plane
    vertices.emplace_back(V3D(0, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    vertices.emplace_back(V3D(1, 1, 0));
    vertices.emplace_back(V3D(1, 0, 1));

    std::vector<uint16_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2});

    // Test constructor taking lvalue references
    TSM_ASSERT_THROWS(
        "non-coplanar points, should throw",
        MeshObject2D(triangles, vertices, Mantid::Kernel::Material()),
        std::invalid_argument &);

    // Test constructor taking rvalue references
    TSM_ASSERT_THROWS("non-coplanar points, should throw",
                      MeshObject2D(std::move(triangles), std::move(vertices),
                                   Mantid::Kernel::Material()),
                      std::invalid_argument &);
  }

  void test_construct() {
    std::vector<V3D> vertices;
    vertices.emplace_back(V3D(-1, 0, 0));
    vertices.emplace_back(V3D(1, 0, 0));
    vertices.emplace_back(V3D(0, 1, 0));
    std::vector<uint16_t> triangles;
    triangles.insert(triangles.end(), {0, 1, 2});

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

  void test_isValid_multi_triangle() {

    // Make 2 Triangles bounded by the specified V3Ds
    auto mesh = makeTrapezoidMesh(V3D{0, 0, 0}, V3D{0, 1, 0}, V3D{1, 1, 0},
                                  V3D{1, 0, 0});
    double delta = 1e-6;
    TS_ASSERT(mesh.isValid(V3D{0, 0, 0}));
    TS_ASSERT(mesh.isValid(V3D{0.5, 0.5, 0}));
    TS_ASSERT(mesh.isValid(V3D{0, 0, 0}));
    TSM_ASSERT("Just outside", !mesh.isValid(V3D{0 - delta, 0, 0}));
    TS_ASSERT(mesh.isValid(V3D{1, 1, 0}));
    TSM_ASSERT("Just outside", !mesh.isValid(V3D{1, 1 + delta, 0}));
  }

  void test_isOnTriangle() {
    auto p1 = V3D{-1, -1, 0};
    auto p2 = V3D{1, -1, 0};
    auto p3 = V3D{0, 1, 0};
    TS_ASSERT(MeshObject2D::isOnTriangle(V3D{0, 0, 0}, p1, p2, p3));
    TS_ASSERT(MeshObject2D::isOnTriangle(p1, p1, p2, p3));
    TS_ASSERT(MeshObject2D::isOnTriangle(p2, p1, p2, p3));
    TS_ASSERT(MeshObject2D::isOnTriangle(p3, p1, p2, p3));
    TS_ASSERT(!MeshObject2D::isOnTriangle(p1 - V3D(0.0001, 0, 0), p1, p2, p3));
    TS_ASSERT(!MeshObject2D::isOnTriangle(p1 - V3D(0, 0.0001, 0), p1, p2, p3));
    TS_ASSERT(!MeshObject2D::isOnTriangle(p2 + V3D(0.0001, 0, 0), p1, p2, p3));
    TS_ASSERT(!MeshObject2D::isOnTriangle(p2 - V3D(0, 0.0001, 0), p1, p2, p3));
    TS_ASSERT(!MeshObject2D::isOnTriangle(p3 + V3D(0.0001, 0, 0), p1, p2, p3));
    TS_ASSERT(!MeshObject2D::isOnTriangle(p3 + V3D(0, 0.0001, 0), p1, p2, p3));
  }

  void test_interceptSurface() {

    auto mesh = makeSimpleTriangleMesh();

    Mantid::Geometry::Track onTargetTrack(V3D{0.5, 0.5, -1},
                                          V3D{0, 0, 1} /*along z*/);
    TS_ASSERT_EQUALS(mesh.interceptSurface(onTargetTrack), 1);
    TS_ASSERT_EQUALS(onTargetTrack.count(), 1);

    Mantid::Geometry::Track missTargetTrack(
        V3D{50, 0.5, -1},
        V3D{0, 0, 1} /*along z*/); // Intersects plane but no triangles
    TS_ASSERT_EQUALS(mesh.interceptSurface(missTargetTrack), 0);
    TS_ASSERT_EQUALS(missTargetTrack.count(), 0);
  }

  void test_equals() {

    auto a = makeTrapezoidMesh(V3D{0, 0, 0}, V3D{0, 1, 0}, V3D{1, 1, 0},
                               V3D{1, 0, 0});
    auto b = makeTrapezoidMesh(V3D{0, 0, 0}, V3D{0, 1, 0}, V3D{1, 1, 0},
                               V3D{1, 0, 0});
    auto c = makeTrapezoidMesh(V3D{0.1, 0, 0}, V3D{0, 1, 0}, V3D{1, 1, 0},
                               V3D{1, 0, 0});
    TS_ASSERT_EQUALS(a, b);
    TS_ASSERT_DIFFERS(a, c);
  }

  void test_clone() {
    auto mesh = makeSimpleTriangleMesh();
    auto clone = mesh.clone();
    TS_ASSERT_EQUALS(*clone, mesh);
  }

  void test_clone_with_material() {
    using namespace Mantid::Kernel;
    auto a = makeSimpleTriangleMesh();
    // Use a different material
    auto b = a.cloneWithMaterial(
        Material("hydrogen", Material::parseChemicalFormula("H"), 3));
    TS_ASSERT_DIFFERS(a, *b);
    // Use same material
    auto c = a.cloneWithMaterial(Material{}); // same empty material
    TS_ASSERT_EQUALS(a, *c);
  }

  void test_solidAngle_side_on() {
    using namespace Mantid::Kernel;

    auto mesh = makeSimpleTriangleMesh();
    auto solidAngle = mesh.solidAngle(
        V3D{0, 2, 0}); // observer is in plane of triangle, outside the triangle

    TS_ASSERT_EQUALS(solidAngle, 0); // seen side-on solid angle is 0
  }

  void test_solidAngle() {

    double expected = M_PI / 3.0; //  0.5 * 4pi/6
    // Unit square at distance 0.5 from observer
    double unitSphereRadius = 1;
    double halfSideLength = unitSphereRadius * sin(M_PI / 4);
    double observerDistance = unitSphereRadius * cos(M_PI / 4);
    auto mesh = makeTrapezoidMesh(
        V3D{-halfSideLength, -halfSideLength, observerDistance},
        V3D{-halfSideLength, halfSideLength, observerDistance},
        V3D{halfSideLength, halfSideLength, observerDistance},
        V3D{halfSideLength, -halfSideLength, observerDistance});
    double solidAngle = mesh.solidAngle(V3D{0, 0, 0});
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);
  }

  void test_boundingBox() {
    auto mesh = makeSimpleTriangleMesh(); // Lies in z plane
    auto boundingBox = mesh.getBoundingBox();
    TS_ASSERT_EQUALS(boundingBox.zMin(), 0);
    TS_ASSERT_DELTA(boundingBox.zMax(), boundingBox.zMin(),
                    MeshObject2D::MinThickness);
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
    TS_ASSERT_THROWS(mesh.generatePointInObject(generator, 10),
                     std::runtime_error &);
  }
  // Characterisation test.
  void test_generatePointInObject_with_active_region_not_supported() {

    // Generating points in a 3D bounding box volume does not make sense for a
    // plane
    auto mesh = makeSimpleTriangleMesh();
    testing::NiceMock<MockRNG> generator;
    Mantid::Geometry::BoundingBox boundingBox;
    TS_ASSERT_THROWS(mesh.generatePointInObject(generator, boundingBox, 10),
                     std::runtime_error &);
  }

  // Characterisation test.
  void test_GetObjGeom_not_implemented() {
    auto mesh = makeSimpleTriangleMesh();
    std::vector<V3D> vectors;
    double radius, height;
    Mantid::Geometry::detail::ShapeInfo::GeometryShape shape;

    TS_ASSERT_THROWS(mesh.GetObjectGeom(shape, vectors, radius, height),
                     std::runtime_error &);
  }

  // Characterisation test.
  void test_draw_not_implemented() {
    auto mesh = makeSimpleTriangleMesh();

    TS_ASSERT_THROWS(mesh.draw(), std::runtime_error &);
  }
  // Characterisation test.
  void test_init_draw_not_implemented() {
    auto mesh = makeSimpleTriangleMesh();

    TS_ASSERT_THROWS(mesh.initDraw(), std::runtime_error &);
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

#endif /* MANTID_GEOMETRY_MESHOBJECT2DTEST_H_ */
