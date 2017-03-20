#ifndef MANTID_KERNEL_NEARESTNEIGHBOURSTEST_H_
#define MANTID_KERNEL_NEARESTNEIGHBOURSTEST_H_

#include <cxxtest/TestSuite.h>
#include "MantidKernel/NearestNeighbours.h"


using Mantid::Kernel::NearestNeighbours;
using namespace Eigen;

class NearestNeighboursTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static NearestNeighboursTest *createSuite() {
    return new NearestNeighboursTest();
  }
  static void destroySuite(NearestNeighboursTest *suite) { delete suite; }

  NearestNeighboursTest() {}

  void test_construct() {
    std::vector<Array3d> pts1 = { Array3d(1, 1, 1), Array3d(2, 2, 2) };
    TS_ASSERT_THROWS_NOTHING( NearestNeighbours<> nn(pts1) );

    std::vector<Array2d> pts2 = { Array2d(1, 1), Array2d(2, 2) };
    TS_ASSERT_THROWS_NOTHING( NearestNeighbours<2> nn(pts2) );

  }

  void test_find_nearest() {
    std::vector<Eigen::Array3d> pts = { Array3d(1, 1, 1), Array3d(2, 2, 2) };
    NearestNeighbours<> nn(pts);

    auto results = nn.findNearest(Array3d(1, 1, 0.9));
    TS_ASSERT_EQUALS(results.size(), 1)

    Eigen::Array3d pos = std::get<0>(results[0]);
    auto index = std::get<1>(results[0]);
    auto dist = std::get<2>(results[0]);
    TS_ASSERT_EQUALS(pos[0], 1)
    TS_ASSERT_EQUALS(pos[1], 1)
    TS_ASSERT_EQUALS(pos[2], 1)
    TS_ASSERT_EQUALS(index, 0)
    TS_ASSERT_DELTA(dist, 0, 0.01)
  }

  void test_find_nearest_2() {
    std::vector<Eigen::Array2d> pts = { Array2d(1, 1), Array2d(2, 2), Array2d(2, 3) };
    NearestNeighbours<2> nn(pts);

    auto results = nn.findNearest(Array2d(1, 0.9), 2);
    TS_ASSERT_EQUALS(results.size(), 2)

    Eigen::Array2d pos = std::get<0>(results[0]);
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
