#ifndef MANTID_HISTOGRAMDATA_BINEDGEVARIANCESTEST_H_
#define MANTID_HISTOGRAMDATA_BINEDGEVARIANCESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdgeStandardDeviations.h"
#include "MantidHistogramData/BinEdgeVariances.h"
#include "MantidHistogramData/PointVariances.h"

using Mantid::HistogramData::BinEdgeStandardDeviations;
using Mantid::HistogramData::BinEdgeVariances;
using Mantid::HistogramData::PointVariances;

class BinEdgeVariancesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinEdgeVariancesTest *createSuite() {
    return new BinEdgeVariancesTest();
  }
  static void destroySuite(BinEdgeVariancesTest *suite) { delete suite; }

  void test_default_constructor() {
    const BinEdgeVariances edges{};
    TS_ASSERT(!edges);
  }

  void test_construct_from_null_PointVariances() {
    const PointVariances points{};
    const BinEdgeVariances edges(points);
    TS_ASSERT(!edges);
  }

  void test_construct_from_empty_PointVariances() {
    PointVariances points(0);
    BinEdgeVariances edges(points);
    TS_ASSERT_EQUALS(edges.size(), 0);
  }

  void test_construct_from_length1_PointVariances() {
    const PointVariances points = {1.0};
    const BinEdgeVariances edges(points);
    TS_ASSERT_EQUALS(edges.size(), 2);
    TS_ASSERT_DELTA(edges[0], 0.5, 1e-14);
    TS_ASSERT_DELTA(edges[1], 1.5, 1e-14);
  }

  void test_construct_from_PointVariances() {
    PointVariances points = {1.0, 3.0, 7.0, 15.0};
    BinEdgeVariances edges(points);
    TS_ASSERT_EQUALS(edges.size(), 5);
    TS_ASSERT_DELTA(edges[0], 0.0, 1e-14);
    TS_ASSERT_DELTA(edges[1], 2.0, 1e-14);
    TS_ASSERT_DELTA(edges[2], 5.0, 1e-14);
    TS_ASSERT_DELTA(edges[3], 11.0, 1e-14);
    TS_ASSERT_DELTA(edges[4], 19.0, 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_BINEDGEVARIANCESTEST_H_ */
