#ifndef MANTID_HISTOGRAMDATA_YVALIDATIONTEST_H_
#define MANTID_HISTOGRAMDATA_YVALIDATIONTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/Counts.h"
#include "MantidHistogramData/Frequencies.h"
#include "MantidHistogramData/HistogramY.h"
#include "MantidHistogramData/YValidation.h"

#include <cfloat>
#include <numeric>

using Mantid::HistogramData::Counts;
using Mantid::HistogramData::Frequencies;
using Mantid::HistogramData::HistogramY;
using Mantid::HistogramData::isValid;

class YValidationTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static YValidationTest *createSuite() { return new YValidationTest(); }
  static void destroySuite(YValidationTest *suite) { delete suite; }

  // FixedLengthVector contains validation, so we need to take a detour for
  // creating a potentially invalid HistogramY.
  HistogramY makeY(std::initializer_list<double> list) {
    std::vector<double> helper(list.size());
    std::iota(helper.begin(), helper.end(), 0.0);
    HistogramY y(helper);
    std::copy(list.begin(), list.end(), y.begin());
    return y;
  }

  void test_works_for_HistogramY() { TS_ASSERT(isValid(HistogramY{1.0, 2.0})); }

  void test_works_for_Counts() { TS_ASSERT(isValid(Counts{1.0, 2.0})); }

  void test_works_for_Frequencies() {
    TS_ASSERT(isValid(Frequencies{1.0, 2.0}));
  }

  void test_length_zero() { TS_ASSERT(isValid(HistogramY(0))); }

  void test_accepts_nan() {
    TS_ASSERT(isValid(makeY({NAN})));
    TS_ASSERT(isValid(makeY({-NAN})));
  }

  void test_detects_inf() {
    TS_ASSERT(!isValid(makeY({INFINITY})));
    TS_ASSERT(!isValid(makeY({-INFINITY})));
  }
};

#endif /* MANTID_HISTOGRAMDATA_YVALIDATIONTEST_H_ */
