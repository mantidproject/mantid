#ifndef MANTID_HISTOGRAMDATA_EVALIDATIONTEST_H_
#define MANTID_HISTOGRAMDATA_EVALIDATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/CountVariances.h"
#include "MantidHistogramData/EValidation.h"
#include "MantidHistogramData/FrequencyStandardDeviations.h"
#include "MantidHistogramData/FrequencyVariances.h"
#include "MantidHistogramData/HistogramE.h"

#include <cfloat>
#include <numeric>

using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::CountVariances;
using Mantid::HistogramData::FrequencyStandardDeviations;
using Mantid::HistogramData::FrequencyVariances;
using Mantid::HistogramData::HistogramE;
using Mantid::HistogramData::isValid;

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

  void test_works_for_CountStandardDeviations() {
    TS_ASSERT(isValid(CountStandardDeviations{1.0, 2.0}));
  }

  void test_works_for_CountVariances() {
    TS_ASSERT(isValid(CountVariances{1.0, 2.0}));
  }

  void test_works_for_FrequencyStandardDeviations() {
    TS_ASSERT(isValid(FrequencyStandardDeviations{1.0, 2.0}));
  }

  void test_works_for_FrequencyVariances() {
    TS_ASSERT(isValid(FrequencyVariances{1.0, 2.0}));
  }

  void test_length_zero() { TS_ASSERT(isValid(HistogramE(0))); }

  void test_detects_negative() {
    TS_ASSERT(!isValid(makeE({-1.0, 1.0, 1.0})));
    TS_ASSERT(!isValid(makeE({1.0, -1.0, 1.0})));
    TS_ASSERT(!isValid(makeE({1.0, 1.0, -1.0})));
  }

  void test_accepts_nan() {
    TS_ASSERT(isValid(makeE({NAN})));
    TS_ASSERT(isValid(makeE({-NAN})));
  }

  void test_detects_inf() {
    TS_ASSERT(!isValid(makeE({INFINITY})));
    TS_ASSERT(!isValid(makeE({-INFINITY})));
  }
};

#endif /* MANTID_HISTOGRAMDATA_EVALIDATIONTEST_H_ */
