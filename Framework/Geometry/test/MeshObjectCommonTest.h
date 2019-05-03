#ifndef MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_
#define MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_

#include <cxxtest/TestSuite.h>
#include <limits>

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

  void test_ray_intersect_triangle_simple() {

    const V3D start{0, 0, -1};
    const V3D direction{0, 0, 1};
    const V3D vertex1{-1, 0, 0};
    const V3D vertex2{1, 0, 0};
    const V3D vertex3{1, 1, 0};
    V3D intersectionPoint;
    TrackDirection entryExitFlag;

    // Direct intersection through triangle body
    auto doesIntersect = MeshObjectCommon::rayIntersectsTriangle(
        start, direction, vertex1, vertex2, vertex3, intersectionPoint,
        entryExitFlag);
    TS_ASSERT(doesIntersect);
    TS_ASSERT_EQUALS(entryExitFlag, TrackDirection::LEAVING);
    TS_ASSERT((start + (direction * 1) - intersectionPoint).norm2() < 1e-9);
  }

  void test_v3d_to_array() {
    std::vector<V3D> input{{1, 2, 3}, {4, 5, 6}};
    auto output = MeshObjectCommon::getVertices(input);
    TS_ASSERT_EQUALS(output, (std::vector<double>{1, 2, 3, 4, 5, 6}));
  }

  void test_ray_intersect_triangle_edge() {

    const V3D direction{0, 0, 1};
    const V3D vertex1{-1, 0, 0};
    const V3D vertex2{1, 0, 0};
    const V3D vertex3{1, 1, 0};
    V3D intersectionPoint;
    TrackDirection entryExitFlag;

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
    TrackDirection entryExitFlag;
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

  void testTooManyVertices() {
    TS_ASSERT_THROWS(MeshObjectCommon::checkVertexLimit(
                         std::numeric_limits<uint32_t>::max()),
                     std::invalid_argument &);
  }
};

#endif /* MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_ */
