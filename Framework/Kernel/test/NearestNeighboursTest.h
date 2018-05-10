#ifndef MANTID_KERNEL_NEARESTNEIGHBOURSTEST_H_
#define MANTID_KERNEL_NEARESTNEIGHBOURSTEST_H_

#include "MantidKernel/NearestNeighbours.h"
#include <cxxtest/TestSuite.h>

using Mantid::Kernel::NearestNeighbours;
using namespace Eigen;

class NearestNeighboursTest : public CxxTest::TestSuite {
public:
  NearestNeighboursTest() {}

  void test_construct() {
    std::vector<Vector3d> pts1 = {Vector3d(1, 1, 1), Vector3d(2, 2, 2)};
    TS_ASSERT_THROWS_NOTHING(NearestNeighbours<3> nn(pts1));

    std::vector<Vector2d> pts2 = {Vector2d(1, 1), Vector2d(2, 2)};
    TS_ASSERT_THROWS_NOTHING(NearestNeighbours<2> nn(pts2));
  }

  void test_find_nearest() {
    std::vector<Eigen::Vector3d> pts = {Vector3d(1, 1, 1), Vector3d(2, 2, 2)};
    NearestNeighbours<3> nn(pts);

    auto results = nn.findNearest(Vector3d(1, 1, 0.9));
    TS_ASSERT_EQUALS(results.size(), 1)

    Eigen::Vector3d pos = std::get<0>(results[0]);
    auto index = std::get<1>(results[0]);
    auto dist = std::get<2>(results[0]);
    TS_ASSERT_EQUALS(pos[0], 1)
    TS_ASSERT_EQUALS(pos[1], 1)
    TS_ASSERT_EQUALS(pos[2], 1)
    TS_ASSERT_EQUALS(index, 0)
    TS_ASSERT_DELTA(dist, 0, 0.01)
  }

  void test_find_nearest_2() {
    std::vector<Eigen::Vector2d> pts = {Vector2d(1, 1), Vector2d(2, 2),
                                        Vector2d(2, 3)};
    NearestNeighbours<2> nn(pts);

    auto results = nn.findNearest(Vector2d(1, 0.9), 2);
    TS_ASSERT_EQUALS(results.size(), 2)

    Eigen::Vector2d pos = std::get<0>(results[0]);
    auto index = std::get<1>(results[0]);
    auto dist = std::get<2>(results[0]);
    TS_ASSERT_EQUALS(pos[0], 1)
    TS_ASSERT_EQUALS(pos[1], 1)
    TS_ASSERT_EQUALS(index, 0)
    TS_ASSERT_DELTA(dist, 0, 0.01)

    pos = std::get<0>(results[1]);
    index = std::get<1>(results[1]);
    dist = std::get<2>(results[1]);
    TS_ASSERT_EQUALS(pos[0], 2)
    TS_ASSERT_EQUALS(pos[1], 2)
    TS_ASSERT_EQUALS(index, 1)
    TS_ASSERT_DELTA(dist, 2.21, 0.01)
  }
};

#endif
