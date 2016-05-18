#ifndef MANTID_HISTOGRAMDATA_COUNTVARIANCESTEST_H_
#define MANTID_HISTOGRAMDATA_COUNTVARIANCESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/CountStandardDeviations.h"
#include "MantidHistogramData/CountVariances.h"

using Mantid::HistogramData::CountStandardDeviations;
using Mantid::HistogramData::CountVariances;

class CountVariancesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static CountVariancesTest *createSuite() { return new CountVariancesTest(); }
  static void destroySuite(CountVariancesTest *suite) { delete suite; }

  void test_construct_default() {
    const CountVariances counts{};
    TS_ASSERT(!counts);
  }

  void test_conversion_identity() {
    const CountVariances variances{1.0, 4.0, 9.0};
    const CountStandardDeviations sigmas(variances);
    const CountVariances result(sigmas);
    TS_ASSERT_EQUALS(result[0], variances[0]);
    TS_ASSERT_EQUALS(result[1], variances[1]);
    TS_ASSERT_EQUALS(result[2], variances[2]);
  }
};

#endif /* MANTID_HISTOGRAMDATA_COUNTVARIANCESTEST_H_ */
