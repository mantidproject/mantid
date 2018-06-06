//----------------
// Includes
//----------------

#include <cxxtest/TestSuite.h>
#include "MantidNexusGeometry/NexusShapeFactory.h"
#include "MantidKernel/V3D.h"

using namespace Mantid::NexusGeometry::NexusShapeFactory;
using Mantid::Kernel::V3D;

class NexusShapeFactoryTest : public CxxTest::TestSuite {
public:
  void test_not_in_plane_if_insufficient_points() {
    std::vector<V3D> points;
    points.push_back(V3D{1, 0, 0});
    points.push_back(V3D{2, 1, 0});
    TS_ASSERT(!pointsCoplanar(points));
  }

  void test_points_not_in_plane_if_colinear() {
    std::vector<V3D> points;
    points.push_back(V3D{1, 0, 0});
    points.push_back(V3D{2, 0, 0});
    points.push_back(V3D{3, 0, 0});
    TS_ASSERT(!pointsCoplanar(points));
  }

  void test_points_in_plane() {
    using namespace Mantid::NexusGeometry::NexusShapeFactory;
    using Mantid::Kernel::V3D;
    std::vector<V3D> points;
    points.push_back(V3D{1, 0, 0});
    points.push_back(V3D{2, 0, 0});
    points.push_back(V3D{3, 0, 0});
    points.push_back(V3D{1, 1, 0});
    TS_ASSERT(pointsCoplanar(points));
  }

  void test_points_not_in_plane() {
    std::vector<V3D> points;
    // Make tetrahedron
    points.push_back(V3D{-1, 0, 0});
    points.push_back(V3D{1, 0, 0});
    points.push_back(V3D{0, 0, -1});
    points.push_back(V3D{0, 1, 0});
    TS_ASSERT(!pointsCoplanar(points));
  }
};
