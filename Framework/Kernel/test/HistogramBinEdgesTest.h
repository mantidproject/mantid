#ifndef MANTID_KERNEL_HISTOGRAM_BINEDGESTEST_H_
#define MANTID_KERNEL_HISTOGRAM_BINEDGESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidKernel/Histogram/BinEdges.h"
#include "MantidKernel/Histogram/Points.h"

using Mantid::Kernel::BinEdges;
using Mantid::Kernel::Points;

class HistogramBinEdgesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static HistogramBinEdgesTest *createSuite() {
    return new HistogramBinEdgesTest();
  }
  static void destroySuite(HistogramBinEdgesTest *suite) { delete suite; }

  void test_construct_from_null_Points() {
    Points points;
    BinEdges edges(points);
    TS_ASSERT(!edges);
  }

  void test_construct_from_empty_Points() {
    Points points(0);
    BinEdges edges(points);
    TS_ASSERT_EQUALS(edges.size(), 0);
  }

  // TODO Is this the correct behavior?
  void test_construct_from_length1_Points() {
    Points points = {1.0};
    BinEdges edges(points);
    TS_ASSERT_EQUALS(edges.size(), 2);
    TS_ASSERT_DELTA(edges[0], 0.5, 1e-14);
    TS_ASSERT_DELTA(edges[1], 1.5, 1e-14);
  }

  void test_construct_from_Points() {
    Points points = {1.0, 3.0, 7.0, 15.0};
    BinEdges edges(points);
    TS_ASSERT_EQUALS(edges.size(), 5);
    TS_ASSERT_DELTA(edges[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(edges[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(edges[2], 5.0, 1e-14);
    TS_ASSERT_DELTA(edges[3], 11.0, 1e-14);
    TS_ASSERT_DELTA(edges[4], 19.0, 1e-14);
  }
};

#endif /* MANTID_KERNEL_HISTOGRAM_BINEDGESTEST_H_ */
