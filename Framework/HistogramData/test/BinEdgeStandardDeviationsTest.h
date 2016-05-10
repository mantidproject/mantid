#ifndef MANTID_HISTOGRAMDATA_BINEDGESTANDARDDEVIATIONSTEST_H_
#define MANTID_HISTOGRAMDATA_BINEDGESTANDARDDEVIATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdgeStandardDeviations.h"
#include "MantidHistogramData/PointStandardDeviations.h"

using Mantid::HistogramData::BinEdgeStandardDeviations;
using Mantid::HistogramData::PointStandardDeviations;

class BinEdgeStandardDeviationsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinEdgeStandardDeviationsTest *createSuite() {
    return new BinEdgeStandardDeviationsTest();
  }
  static void destroySuite(BinEdgeStandardDeviationsTest *suite) {
    delete suite;
  }

  void test_default_constructor() {
    const BinEdgeStandardDeviations edges{};
    TS_ASSERT(!edges);
  }

  void test_construct_from_null_PointStandardDeviations() {
    const PointStandardDeviations points{};
    const BinEdgeStandardDeviations edges(points);
    TS_ASSERT(!edges);
  }

  void test_construct_from_empty_PointStandardDeviations() {
    PointStandardDeviations points(0);
    BinEdgeStandardDeviations edges(points);
    TS_ASSERT_EQUALS(edges.size(), 0);
  }

  void test_construct_from_length1_PointStandardDeviations() {
    const PointStandardDeviations points = {1.0};
    const BinEdgeStandardDeviations edges(points);
    TS_ASSERT_EQUALS(edges.size(), 2);
    TS_ASSERT_DELTA(edges[0], 0.5, 1e-14);
    TS_ASSERT_DELTA(edges[1], 1.5, 1e-14);
  }

  void test_construct_from_PointStandardDeviations() {
    PointStandardDeviations points = {1.0, 3.0, 7.0, 15.0};
    BinEdgeStandardDeviations edges(points);
    TS_ASSERT_EQUALS(edges.size(), 5);
    TS_ASSERT_DELTA(edges[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(edges[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(edges[2], 5.0, 1e-14);
    TS_ASSERT_DELTA(edges[3], 11.0, 1e-14);
    TS_ASSERT_DELTA(edges[4], 19.0, 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_BINEDGESTANDARDDEVIATIONSTEST_H_ */
