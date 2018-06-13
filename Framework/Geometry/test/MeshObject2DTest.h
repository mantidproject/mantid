#ifndef MANTID_GEOMETRY_MESH2DOBJECTTEST_H_
#define MANTID_GEOMETRY_MESH2DOBJECTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidGeometry/Objects/Track.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"

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
};

#endif /* MANTID_GEOMETRY_MESH2DOBJECTTEST_H_ */
