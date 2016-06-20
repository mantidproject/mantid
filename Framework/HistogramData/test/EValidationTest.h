#ifndef MANTID_HISTOGRAMDATA_EVALIDATIONTEST_H_
#define MANTID_HISTOGRAMDATA_EVALIDATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/BinEdgeStandardDeviations.h"
#include "MantidHistogramData/BinEdgeVariances.h"
#include "MantidHistogramData/EValidation.h"
#include "MantidHistogramData/HistogramE.h"
#include "MantidHistogramData/PointStandardDeviations.h"
#include "MantidHistogramData/PointVariances.h"

#include <cfloat>
#include <numeric>

using Mantid::HistogramData::BinEdgeStandardDeviations;
using Mantid::HistogramData::BinEdgeVariances;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::isValid;
using Mantid::HistogramData::PointStandardDeviations;
using Mantid::HistogramData::PointVariances;

class EValidationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static EValidationTest *createSuite() { return new EValidationTest(); }
  static void destroySuite(EValidationTest *suite) { delete suite; }

  // FixedLengthVector contains validation, so we need to take a detour for
  // creating a potentially invalid HistogramE.
  HistogramE makeE(std::initializer_list<double> list) {
    std::vector<double> helper(list.size());
    std::iota(helper.begin(), helper.end(), 0.0);
    HistogramE x(helper);
    std::copy(list.begin(), list.end(), x.begin());
    return x;
  }

  void test_works_for_HistogramE() { TS_ASSERT(isValid(HistogramE{1.0, 2.0})); }

  void test_works_for_BinEdgeStandardDeviations() {
    TS_ASSERT(isValid(BinEdgeStandardDeviations{1.0, 2.0}));
  }

  void test_works_for_BinEdgeVariances() {
    TS_ASSERT(isValid(BinEdgeVariances{1.0, 2.0}));
  }

  void test_works_for_PointStandardDeviations() {
    TS_ASSERT(isValid(PointStandardDeviations{1.0, 2.0}));
  }

  void test_works_for_PointVariances() {
    TS_ASSERT(isValid(PointVariances{1.0, 2.0}));
  }

  void test_length_zero() {
    TS_ASSERT(isValid(HistogramE(0)));
  }

  void test_detects_negative() {
    TS_ASSERT(!isValid(makeE({-1.0, 1.0, 1.0})));
    TS_ASSERT(!isValid(makeE({1.0, -1.0, 1.0})));
    TS_ASSERT(!isValid(makeE({1.0, 1.0, -1.0})));
  }

  void test_detects_nan() {
    TS_ASSERT(!isValid(makeE({NAN})));
    TS_ASSERT(!isValid(makeE({-NAN})));
  }

  void test_detects_inf() {
    TS_ASSERT(!isValid(makeE({INFINITY})));
    TS_ASSERT(!isValid(makeE({-INFINITY})));
  }
};

#endif /* MANTID_HISTOGRAMDATA_EVALIDATIONTEST_H_ */
