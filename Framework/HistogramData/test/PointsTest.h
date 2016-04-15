#ifndef MANTID_HISTOGRAM_POINTSTEST_H_
#define MANTID_HISTOGRAM_POINTSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogram/Points.h"
#include "MantidHistogram/BinEdges.h"

using Mantid::Histogram::Points;
using Mantid::Histogram::BinEdges;

class PointsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointsTest *createSuite() { return new PointsTest(); }
  static void destroySuite(PointsTest *suite) { delete suite; }

  void test_construct_default() {
    const Points points{};
    TS_ASSERT(!points);
  }

  void test_construct_from_null_BinEdges() {
    const BinEdges edges{};
    const Points points(edges);
    TS_ASSERT(!points);
  }

  void test_construct_from_empty_BinEdges() {
    const BinEdges edges(0);
    const Points points(edges);
    TS_ASSERT_EQUALS(points.size(), 0);
  }

  void test_construct_from_invalid_BinEdges() {
    const BinEdges edges(1);
    TS_ASSERT_THROWS(Points points(edges), std::logic_error);
  }

  void test_construct_from_BinEdges() {
    const BinEdges edges{1.0, 3.0, 7.0, 15.0};
    const Points points(edges);
    TS_ASSERT_EQUALS(points.size(), 3);
    TS_ASSERT_DELTA(points[0], 2.0, 1e-14);
    TS_ASSERT_DELTA(points[1], 5.0, 1e-14);
    TS_ASSERT_DELTA(points[2], 11.0, 1e-14);
  }
};

#endif /* MANTID_HISTOGRAM_POINTSTEST_H_ */
