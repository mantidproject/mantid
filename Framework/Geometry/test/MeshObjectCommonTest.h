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
    double expected = M_PI / 3.0; //  0.5 * 4pi/6
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
    double solidAngle =
        MeshObjectCommon::solidAngle(V3D{0, 0, 0}, triangles, vertices);
    TS_ASSERT_DELTA(solidAngle, expected, 1e-3);
  }
};

#endif /* MANTID_GEOMETRY_MESHOBJECTCOMMONTEST_H_ */
