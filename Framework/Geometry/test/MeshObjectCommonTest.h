#ifndef MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_
#define MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidGeometry/Objects/MeshObjectCommon.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::Geometry;
using Mantid::Kernel::V3D;

class MeshObjectCommonTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static MeshObjectCommonTest *createSuite() {
    return new MeshObjectCommonTest();
  }
  static void destroySuite(MeshObjectCommonTest *suite) { delete suite; }

  void test_v3d_to_array() {
    std::vector<V3D> input{{1, 2, 3}, {4, 5, 6}};
    auto output = MeshObjectCommon::getVertices(input);
    TS_ASSERT_EQUALS(output, (std::vector<double>{1, 2, 3, 4, 5, 6}));
  }
  void test_solidAngle_side_on() {
    using namespace Mantid::Kernel;
    std::vector<V3D> vertices = {V3D{-1, 0, 0}, V3D{1, 0, 0}, V3D{1, 1, 0}};
    std::vector<uint16_t> triangles{0, 1, 2};
    auto solidAngle = MeshObjectCommon::solidAngle(
        V3D{0, 2, 0}, // observer is in plane of triangle, outside the
        triangles, vertices);
    TS_ASSERT_EQUALS(solidAngle, 0); // seen side-on solid angle is 0
  }
  void test_triangle_solid_angle() {

    // Unit square inside unit cube. Any cube face will have solid angle 1/6th
    // of total 4pi steradians. Observer at origin.

    double expected = 2.0 * M_PI / 3.0; //  4pi/6
    // Unit square at distance cos(M_PI / 4) from observer
    double unitSphereRadius = 1;
    double halfSideLength = unitSphereRadius * sin(M_PI / 4);
    double observerDistance = unitSphereRadius * cos(M_PI / 4);
    std::vector<V3D> vertices = {
        V3D{-halfSideLength, -halfSideLength, observerDistance},
        V3D{-halfSideLength, halfSideLength, observerDistance},
        V3D{halfSideLength, halfSideLength, observerDistance},
        V3D{halfSideLength, -halfSideLength, observerDistance}};
    std::vector<uint16_t> triangles{0, 1, 2, 2, 3, 0};
    double solidAngle =
        MeshObjectCommon::solidAngle(V3D{0, 0, 0}, triangles, vertices);
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);

    // Solid angle is the same from other side of square.
    solidAngle = MeshObjectCommon::solidAngle(V3D{0, 0, 2 * observerDistance},
                                              triangles, vertices);
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);
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
        V3D{-halfSideLength, -halfSideLength, observerDistance},
        V3D{-halfSideLength, halfSideLength, observerDistance},
        V3D{halfSideLength, halfSideLength, observerDistance},
        V3D{halfSideLength, -halfSideLength, observerDistance}};
    std::vector<uint16_t> triangles{0, 1, 2, 2, 3, 0};
    // Scaling square uniformly (and reducing distance to origin by same
    // factory), yields same angular area 4pi/6
    V3D scaleFactor{0.5, 0.5, 0.5};
    double solidAngle = MeshObjectCommon::solidAngle(V3D{0, 0, 0}, triangles,
                                                     vertices, scaleFactor);
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);

    // Scaling square uniformly (and increasing distance to origin by same
    // factory), yields same angular area 4pi/6
    scaleFactor = {2, 2, 2};
    solidAngle = MeshObjectCommon::solidAngle(V3D{0, 0, 0}, triangles, vertices,
                                              scaleFactor);
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);
  }

  void test_ray_intersect_triangle_simple() {

    const V3D start{0, 0, -1};
    const V3D direction{0, 0, 1};
    const V3D vertex1{-1, 0, 0};
    const V3D vertex2{1, 0, 0};
    const V3D vertex3{1, 1, 0};
    V3D intersectionPoint;
    int entryExitFlag;

    // Direct intersection through triangle body
    auto doesIntersect = MeshObjectCommon::rayIntersectsTriangle(
        start, direction, vertex1, vertex2, vertex3, intersectionPoint,
        entryExitFlag);
    TS_ASSERT(doesIntersect);
    TS_ASSERT_EQUALS(entryExitFlag, -1);
    TS_ASSERT((start + (direction * 1) - intersectionPoint).norm2() < 1e-9);
  }

  void test_ray_intersect_triangle_edge() {

    const V3D direction{0, 0, 1};
    const V3D vertex1{-1, 0, 0};
    const V3D vertex2{1, 0, 0};
    const V3D vertex3{1, 1, 0};
    V3D intersectionPoint;
    int entryExitFlag;

    // Test ray going through vertex of triangle
    V3D start = vertex1 - direction;
    auto doesIntersect = MeshObjectCommon::rayIntersectsTriangle(
        start, direction, vertex1, vertex2, vertex3, intersectionPoint,
        entryExitFlag);
    TS_ASSERT(doesIntersect);

    // Check another vertex
    start = vertex3 - direction;
    doesIntersect = MeshObjectCommon::rayIntersectsTriangle(
        start, direction, vertex1, vertex2, vertex3, intersectionPoint,
        entryExitFlag);
    TS_ASSERT(doesIntersect);

    // Check an edge
    start = (vertex1 + vertex2) / 2 - direction; // along edge
    doesIntersect = MeshObjectCommon::rayIntersectsTriangle(
        start, direction, vertex1, vertex2, vertex3, intersectionPoint,
        entryExitFlag);
    TS_ASSERT(doesIntersect);

    // Sanity check just outside.
    start = (vertex1 + vertex2) / 2 - direction; // along edge
    start += V3D(0, -1e-6, 0);                   // minor shift down in y
    doesIntersect = MeshObjectCommon::rayIntersectsTriangle(
        start, direction, vertex1, vertex2, vertex3, intersectionPoint,
        entryExitFlag);
    TS_ASSERT(!doesIntersect);
  }

  void test_no_ray_intersect_triangle_when_triangle_behind() {
    V3D start{0, 0, -1};
    V3D direction{0, 0, 1};
    const V3D vertex1{-1, 0, 0};
    const V3D vertex2{1, 0, 0};
    const V3D vertex3{1, 1, 0};
    V3D intersectionPoint;
    int entryExitFlag;
    // Triangle now behind start. Should not intersect
    start = V3D{0, 0, 10};
    auto doesIntersect = MeshObjectCommon::rayIntersectsTriangle(
        start, direction, vertex1, vertex2, vertex3, intersectionPoint,
        entryExitFlag);
    TS_ASSERT(!doesIntersect);
  }
  void test_isOnTriangle() {
    auto p1 = V3D{-1, -1, 0};
    auto p2 = V3D{1, -1, 0};
    auto p3 = V3D{0, 1, 0};
    TS_ASSERT(MeshObjectCommon::isOnTriangle(V3D{0, 0, 0}, p1, p2, p3));
    TS_ASSERT(MeshObjectCommon::isOnTriangle(p1, p1, p2, p3));
    TS_ASSERT(MeshObjectCommon::isOnTriangle(p2, p1, p2, p3));
    TS_ASSERT(MeshObjectCommon::isOnTriangle(p3, p1, p2, p3));
    TS_ASSERT(
        !MeshObjectCommon::isOnTriangle(p1 - V3D(0.0001, 0, 0), p1, p2, p3));
    TS_ASSERT(
        !MeshObjectCommon::isOnTriangle(p1 - V3D(0, 0.0001, 0), p1, p2, p3));
    TS_ASSERT(
        !MeshObjectCommon::isOnTriangle(p2 + V3D(0.0001, 0, 0), p1, p2, p3));
    TS_ASSERT(
        !MeshObjectCommon::isOnTriangle(p2 - V3D(0, 0.0001, 0), p1, p2, p3));
    TS_ASSERT(
        !MeshObjectCommon::isOnTriangle(p3 + V3D(0.0001, 0, 0), p1, p2, p3));
    TS_ASSERT(
        !MeshObjectCommon::isOnTriangle(p3 + V3D(0, 0.0001, 0), p1, p2, p3));
  }
};

#endif /* MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_ */
