#ifndef MANTID_HISTOGRAMDATA_XVALIDATIONTEST_H_
#define MANTID_HISTOGRAMDATA_XVALIDATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/XValidation.h"

#include <cfloat>

using Mantid::HistogramData::BinEdges;
using Mantid::HistogramData::HistogramX;
using Mantid::HistogramData::isValid;
using Mantid::HistogramData::Points;

class XValidationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static XValidationTest *createSuite() { return new XValidationTest(); }
  static void destroySuite(XValidationTest *suite) { delete suite; }

  void test_works_for_HistogramX() {
    TS_ASSERT(isValid(HistogramX{1.0, 2.0}));
    TS_ASSERT(!isValid(HistogramX(2)));
  }

  void test_works_for_BinEdges() {
    TS_ASSERT(isValid(BinEdges{1.0, 2.0}));
    TS_ASSERT(!isValid(BinEdges(2)));
  }

  void test_works_for_Points() {
    TS_ASSERT(isValid(Points{1.0, 2.0}));
    TS_ASSERT(!isValid(Points(2)));
  }

  void test_detects_zero_width() {
    TS_ASSERT(!isValid(HistogramX{1.0, 2.0, 2.0, 3.0}));
  }

  void test_detects_non_increasing() {
    TS_ASSERT(!isValid(HistogramX{1.0, 3.0, 2.0, 4.0}));
  }

  void test_detects_nan() {
    TS_ASSERT(!isValid(HistogramX{NAN}));
    TS_ASSERT(!isValid(HistogramX{NAN, 1.0}));
    TS_ASSERT(!isValid(HistogramX{NAN, -1.0}));
    TS_ASSERT(!isValid(HistogramX{1.0, NAN}));
    TS_ASSERT(!isValid(HistogramX{-1.0, NAN}));
  }

  void test_detects_inf() {
    TS_ASSERT(!isValid(HistogramX{INFINITY}));
    TS_ASSERT(!isValid(HistogramX{-INFINITY}));
    TS_ASSERT(!isValid(HistogramX{-INFINITY, 0.0}));
    TS_ASSERT(!isValid(HistogramX{0.0, INFINITY}));
    TS_ASSERT(!isValid(HistogramX{-INFINITY, INFINITY}));
    TS_ASSERT(isValid(HistogramX{-DBL_MAX / 2.0, DBL_MAX / 2.0}));
    TS_ASSERT(!isValid(HistogramX{-DBL_MAX, DBL_MAX}));
  }

  void test_denormal() {
    // Denormal values are ok
    TS_ASSERT(isValid(HistogramX{0.0}));
    TS_ASSERT(isValid(HistogramX{DBL_MIN / 2.0}));
    TS_ASSERT(isValid(HistogramX{DBL_MIN / 2.0, 1.0}));
    TS_ASSERT(isValid(HistogramX{-1.0, DBL_MIN / 2.0}));
  }

  void test_detects_denormal() {
    // Denormal differences are not ok
    TS_ASSERT(isValid(HistogramX{0.0, DBL_MIN}));
    TS_ASSERT(!isValid(HistogramX{0.0, DBL_MIN / 2.0}));
    TS_ASSERT(!isValid(HistogramX{DBL_MIN / 2.0, DBL_MIN}));
  }
};

#endif /* MANTID_HISTOGRAMDATA_XVALIDATIONTEST_H_ */
