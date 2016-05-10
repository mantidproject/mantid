#ifndef MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONSTEST_H_
#define MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdgeStandardDeviations.h"
#include "MantidHistogramData/PointStandardDeviations.h"

using Mantid::HistogramData::BinEdgeStandardDeviations;
using Mantid::HistogramData::PointStandardDeviations;

class PointStandardDeviationsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static PointStandardDeviationsTest *createSuite() {
    return new PointStandardDeviationsTest();
  }
  static void destroySuite(PointStandardDeviationsTest *suite) { delete suite; }

  void test_construct_default() {
    const PointStandardDeviations points{};
    TS_ASSERT(!points);
  }

  void test_construct_from_null_BinEdgeStandardDeviations() {
    const BinEdgeStandardDeviations edges{};
    const PointStandardDeviations points(edges);
    TS_ASSERT(!points);
  }

  void test_construct_from_empty_BinEdgeStandardDeviations() {
    const BinEdgeStandardDeviations edges(0);
    const PointStandardDeviations points(edges);
    TS_ASSERT_EQUALS(points.size(), 0);
  }

  void test_construct_from_invalid_BinEdgeStandardDeviations() {
    const BinEdgeStandardDeviations edges(1);
    TS_ASSERT_THROWS(PointStandardDeviations points(edges), std::logic_error);
  }

  void test_construct_from_BinEdgeStandardDeviations() {
    const BinEdgeStandardDeviations edges{1.0, 3.0, 7.0, 15.0};
    const PointStandardDeviations points(edges);
    TS_ASSERT_EQUALS(points.size(), 3);
    TS_ASSERT_DELTA(points[0], 2.0, 1e-14);
    TS_ASSERT_DELTA(points[1], 5.0, 1e-14);
    TS_ASSERT_DELTA(points[2], 11.0, 1e-14);
  }
};

#endif /* MANTID_HISTOGRAMDATA_POINTSTANDARDDEVIATIONSTEST_H_ */
