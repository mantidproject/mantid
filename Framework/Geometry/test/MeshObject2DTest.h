#ifndef MANTID_GEOMETRY_MESH2DOBJECTTEST_H_
#define MANTID_GEOMETRY_MESH2DOBJECTTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/MeshObject2D.h"
#include "MantidKernel/Material.h"
#include "MantidKernel/V3D.h"

using Mantid::Geometry::MeshObject2D;
using Mantid::Kernel::V3D;

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
};

#endif /* MANTID_GEOMETRY_MESH2DOBJECTTEST_H_ */
