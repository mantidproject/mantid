#ifndef MANTID_HISTOGRAMDATA_XVALIDATIONTEST_H_
#define MANTID_HISTOGRAMDATA_XVALIDATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdges.h"
#include "MantidHistogramData/Points.h"
#include "MantidHistogramData/XValidation.h"

#include <numeric>

using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::Points;
using Mantid::HistogramData::isValid;

class XValidationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static XValidationTest *createSuite() { return new XValidationTest(); }
  static void destroySuite(XValidationTest *suite) { delete suite; }

  // FixedLengthVector contains validation, so we need to take a detour for
  // creating a potentially invalid HistogramX.
  HistogramX makeX(std::initializer_list<double> list) {
    std::vector<double> helper(list.size());
    std::iota(helper.begin(), helper.end(), 0.0);
    HistogramX x(helper);
    std::copy(list.begin(), list.end(), x.begin());
    return x;
  }

  void test_works_for_HistogramX() {
    HistogramX data{1.0, 2.0};
    TS_ASSERT(isValid(data));
    data[1] = data[0];
    TS_ASSERT(!isValid(data));
  }

  void test_works_for_BinEdges() {
    BinEdges data{1.0, 2.0};
    TS_ASSERT(isValid(data));
    data.mutableRawData()[1] = data[0];
    TS_ASSERT(!isValid(data));
  }

  void test_works_for_Points() {
    Points data{1.0, 2.0};
    TS_ASSERT(isValid(data));
    data.mutableRawData()[1] = data[0];
    TS_ASSERT(!isValid(data));
  }

  void test_detects_zero_width() {
    TS_ASSERT(!isValid(makeX({1.0, 2.0, 2.0, 3.0})));
  }

  void test_detects_non_increasing() {
    TS_ASSERT(!isValid(makeX({1.0, 3.0, 2.0, 4.0})));
  }

  void test_accepts_nan() {
    TS_ASSERT(isValid(makeX({NAN})));
    TS_ASSERT(isValid(makeX({NAN, 1.0})));
    TS_ASSERT(isValid(makeX({NAN, -1.0})));
    TS_ASSERT(isValid(makeX({1.0, NAN})));
    TS_ASSERT(isValid(makeX({-1.0, NAN})));
  }

  void test_detects_non_boundary_nan() {
    TS_ASSERT(!isValid(makeX({-1.0, NAN, 1.0})));
    TS_ASSERT(isValid(makeX({NAN, -1.0, 0.0, 1.0})));
    TS_ASSERT(!isValid(makeX({NAN, -1.0, NAN, 1.0})));
    TS_ASSERT(isValid(makeX({-1.0, 0.0, 1.0, NAN})));
    TS_ASSERT(!isValid(makeX({-1.0, NAN, 1.0, NAN})));
  }

  void test_accepts_inf() {
    TS_ASSERT(isValid(makeX({INFINITY})));
    TS_ASSERT(isValid(makeX({-INFINITY})));
    TS_ASSERT(isValid(makeX({-INFINITY, 0.0})));
    TS_ASSERT(isValid(makeX({0.0, INFINITY})));
    TS_ASSERT(isValid(makeX({-INFINITY, INFINITY})));
    TS_ASSERT(isValid(makeX({-DBL_MAX / 2.0, DBL_MAX / 2.0})));
    TS_ASSERT(isValid(makeX({-DBL_MAX, DBL_MAX})));
  }

  void test_detects_non_increasing_inf() {
    // INF is ok, but order must be correct
    TS_ASSERT(!isValid(makeX({0.0, -INFINITY})));
    TS_ASSERT(!isValid(makeX({INFINITY, 0.0})));
    TS_ASSERT(!isValid(makeX({INFINITY, -INFINITY})));
    TS_ASSERT(!isValid(makeX({DBL_MAX, -DBL_MAX})));
  }

  void test_denormal() {
    // Denormal values are ok
    TS_ASSERT(isValid(makeX({0.0})));
    TS_ASSERT(isValid(makeX({DBL_MIN / 2.0})));
    TS_ASSERT(isValid(makeX({DBL_MIN / 2.0, 1.0})));
    TS_ASSERT(isValid(makeX({-1.0, DBL_MIN / 2.0})));
  }

  void test_detects_denormal() {
    // Denormal differences are not ok
    TS_ASSERT(isValid(makeX({0.0, DBL_MIN})));
    TS_ASSERT(!isValid(makeX({0.0, DBL_MIN / 2.0})));
    TS_ASSERT(!isValid(makeX({DBL_MIN / 2.0, DBL_MIN})));
  }
};

#endif /* MANTID_HISTOGRAMDATA_XVALIDATIONTEST_H_ */
