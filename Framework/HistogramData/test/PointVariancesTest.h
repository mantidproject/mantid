#ifndef MANTID_HISTOGRAMDATA_POINTVARIANCESTEST_H_
#define MANTID_HISTOGRAMDATA_POINTVARIANCESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdgeVariances.h"
#include "MantidHistogramData/PointStandardDeviations.h"
#include "MantidHistogramData/PointVariances.h"

using Mantid::HistogramData::BinEdgeVariances;
using Mantid::HistogramData::PointStandardDeviations;
using Mantid::HistogramData::PointVariances;

class PointVariancesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointVariancesTest *createSuite() { return new PointVariancesTest(); }
  static void destroySuite(PointVariancesTest *suite) { delete suite; }

  void test_construct_default() {
    const PointVariances points{};
    TS_ASSERT(!points);
  }

  void test_construct_from_null_BinEdgeVariances() {
    const BinEdgeVariances edges{};
    const PointVariances points(edges);
    TS_ASSERT(!points);
  }

  void test_construct_from_empty_BinEdgeVariances() {
    const BinEdgeVariances edges(0);
    const PointVariances points(edges);
    TS_ASSERT_EQUALS(points.size(), 0);
  }

  void test_construct_from_invalid_BinEdgeVariances() {
    const BinEdgeVariances edges(1);
    TS_ASSERT_THROWS(PointVariances points(edges), std::logic_error);
  }

  void test_construct_from_BinEdgeVariances() {
    const BinEdgeVariances edges{1.0, 3.0, 7.0, 15.0};
    const PointVariances points(edges);
    TS_ASSERT_EQUALS(points.size(), 3);
    TS_ASSERT_DELTA(points[0], 2.0, 1e-14);
    TS_ASSERT_DELTA(points[1], 5.0, 1e-14);
    TS_ASSERT_DELTA(points[2], 11.0, 1e-14);
  }

  void test_conversion_identity() {
    const PointVariances variances{1.0, 4.0, 9.0};
    const PointStandardDeviations sigmas(variances);
    const PointVariances result(sigmas);
    TS_ASSERT_EQUALS(result[0], variances[0]);
    TS_ASSERT_EQUALS(result[1], variances[1]);
    TS_ASSERT_EQUALS(result[2], variances[2]);
  }
};

#endif /* MANTID_HISTOGRAMDATA_POINTVARIANCESTEST_H_ */
