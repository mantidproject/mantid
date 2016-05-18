#ifndef MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONSTEST_H_
#define MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONSTEST_H_

#include <cxxtest/TestSuite.h>

#include "MantidHistogramData/FrequencyStandardDeviations.h"

using Mantid::HistogramData::FrequencyStandardDeviations;

class FrequencyStandardDeviationsTest : public CxxTest::TestSuite {
public:
  // This pair of boilerplate methods prevent the suite being created statically
  // This means the constructor isn't called when running other tests
  static FrequencyStandardDeviationsTest *createSuite() {
    return new FrequencyStandardDeviationsTest();
  }
  static void destroySuite(FrequencyStandardDeviationsTest *suite) {
    delete suite;
  }

  void test_construct_default() {
    const FrequencyStandardDeviations points{};
    TS_ASSERT(!points);
  }
};

#endif /* MANTID_HISTOGRAMDATA_FREQUENCYSTANDARDDEVIATIONSTEST_H_ */
