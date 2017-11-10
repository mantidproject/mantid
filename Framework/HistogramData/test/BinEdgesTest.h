#ifndef MANTID_HISTOGRAMDATA_BINEDGESTEST_H_
#define MANTID_HISTOGRAMDATA_BINEDGESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Points.h"

using namespace Mantid;
using namespace HistogramData;

class BinEdgesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static BinEdgesTest *createSuite() { return new BinEdgesTest(); }
  static void destroySuite(BinEdgesTest *suite) { delete suite; }

  void test_has_correct_mixins() {
    BinEdges data;
    TS_ASSERT_THROWS_NOTHING(UNUSED_ARG(
        (dynamic_cast<detail::VectorOf<BinEdges, HistogramX> &>(data))));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Iterable<BinEdges> &>(data)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Offsetable<BinEdges> &>(data)));
    TS_ASSERT_THROWS_NOTHING(
        UNUSED_ARG(dynamic_cast<detail::Scalable<BinEdges> &>(data)));
  }

  void test_default_constructor() {
    const BinEdges edges{};
    TS_ASSERT(!edges);
  }

  void test_construct_from_null_Points() {
    const Points points{};
    const BinEdges edges(points);
    TS_ASSERT(!edges);
  }

  void test_construct_from_empty_Points() {
    Points points(0);
    BinEdges edges(points);
    TS_ASSERT_EQUALS(edges.size(), 0);
  }

  void test_construct_from_length1_Points() {
    const Points points = {1.0};
    const BinEdges edges(points);
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

#endif /* MANTID_HISTOGRAMDATA_BINEDGESTEST_H_ */
