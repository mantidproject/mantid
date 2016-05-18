#ifndef MANTID_HISTOGRAMDATA_FREQUENCYVARIANCESTEST_H_
#define MANTID_HISTOGRAMDATA_FREQUENCYVARIANCESTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FrequencyStandardDeviations.h"
#include "MantidHistogramData/FrequencyVariances.h"

using Mantid::HistogramData::FrequencyStandardDeviations;
using Mantid::HistogramData::FrequencyVariances;

class FrequencyVariancesTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FrequencyVariancesTest *createSuite() {
    return new FrequencyVariancesTest();
  }
  static void destroySuite(FrequencyVariancesTest *suite) { delete suite; }

  void test_construct_default() {
    const FrequencyVariances frequencies{};
    TS_ASSERT(!frequencies);
  }

  void test_conversion_identity() {
    const FrequencyVariances variances{1.0, 4.0, 9.0};
    const FrequencyStandardDeviations sigmas(variances);
    const FrequencyVariances result(sigmas);
    TS_ASSERT_EQUALS(result[0], variances[0]);
    TS_ASSERT_EQUALS(result[1], variances[1]);
    TS_ASSERT_EQUALS(result[2], variances[2]);
  }
};

#endif /* MANTID_HISTOGRAMDATA_FREQUENCYVARIANCESTEST_H_ */
